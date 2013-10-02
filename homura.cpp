#include <iostream>
#include <sstream>
#include <cstdint>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <vector>
#include <stdlib.h>

#include <cp210x.h>


static const int log_level = getenv("DEBUG_HOMURA") != 0;

using boost::asio::ip::tcp;

class base_stream
{
public:
	typedef boost::function<void (int status, size_t len)> send_callback;
	typedef boost::function<int (void *data, size_t len, send_callback callback)> sender_t;

	~base_stream() {
		std::cerr << "~base_stream" << std::endl;	
	}

	boost::signals2::signal<void (void *data, size_t len)> data_received;
	virtual int send(void *data, size_t len, send_callback callback) = 0;
};

class data_stream : public base_stream
{
public:
	data_stream(sender_t _sender, uint32_t _index):sender(_sender) {
	
	}
	
	~data_stream() {
		if(log_level) std::cerr << "~data_stream" << std::endl;
	}
	
	virtual int send(void *data, size_t len, base_stream::send_callback callback) {
		return sender(data,len,callback);
	}	
private:
	sender_t sender;
};

template<class Protocol>
class stream_connection 
:public boost::enable_shared_from_this< stream_connection<Protocol> >
{
	stream_connection(boost::asio::io_service& io_service):socket(io_service) {
		
	}

	uint8_t read_buf[256];
	
	typename Protocol::socket socket;
	typename Protocol::endpoint endpoint;
	
	boost::signals2::connection connection;	
public:
	boost::signals2::signal<void ()> disconnected;

	typedef boost::shared_ptr< stream_connection<Protocol> > pointer;

	~stream_connection() {
		std::cerr << "~stream_connection" << std::endl;
		disconnected();
	}

	static pointer create(boost::asio::io_service& io_service) {
		return pointer(new stream_connection<Protocol>(io_service));
	}

	typename Protocol::socket& get_socket() {
		return socket;
	}
  
	typename Protocol::endpoint& get_endpoint() {
		return endpoint;  
	}  

	void net_to_device(boost::shared_ptr<base_stream> stream) {
		pointer shared = this->shared_from_this();
		boost::asio::async_read(socket,boost::asio::buffer(read_buf),
			//check callback
			[](const boost::system::error_code &error,
			   size_t bytes_transferred) {
				if(log_level) std::cerr << "check: " << bytes_transferred << std::endl;
				return !!error || bytes_transferred > 1;
			},
			//read callback
			[=](const boost::system::error_code &error,
				size_t bytes_transferred) {
				if(log_level) {
					std::cerr << "sender: "
				              << error.message() <<" : "
				              << bytes_transferred << std::endl;
				}
				 
				if(!error) {
					stream->send(read_buf,bytes_transferred,[=](int status, size_t len) {
						if(log_level) std::cerr << "data_sent: " << status << ": " << len << " bytes" << std::endl;
						shared->net_to_device(stream);					
					});					
				} else {
					shared->connection.disconnect();
					//shared.reset();
				}					
			});
	}

	void start(boost::shared_ptr<base_stream> stream) {
		net_to_device(stream);
		
		pointer shared = this->shared_from_this();		
		auto receiver = [shared](void *data, size_t len) {
			if(log_level) {
				auto bytes = usb::format_bytes(data,len);
				std::cerr << "receiver: " << bytes.get() << std::endl;
			}
			
			auto c = shared->connection;
			boost::asio::async_write(shared->socket,boost::asio::buffer(data,len),
				[c](const boost::system::error_code &error,
				    size_t bytes_transferred) {
					
					if(log_level) {
						std::cerr << "async_write handler: " 
						          << error.message() << " : "
						          << bytes_transferred << std::endl;
					}
					
					if(error) {
						c.disconnect();
					}
				});
			};
		
		connection = stream->data_received.connect(receiver);
	}
};

template<class Protocol>
class stream_server 
{
	typename Protocol::acceptor acceptor;
	
	boost::shared_ptr<base_stream> stream;
	
	void async_accept()
	{
		typedef stream_connection<Protocol> connection;
	
		auto c = connection::create(acceptor.get_io_service());
		 
		acceptor.async_accept(c->get_socket(),c->get_endpoint(),
		 [=,this](const boost::system::error_code &error) {
			std::cerr << "stream_server accept_handler: "
			          << error.message()
			          << " | endpoint: " 
			          << c->get_endpoint() << std::endl;
			
			if(!error) {
				c->start(this->stream);
				this->async_accept();				
			}
		 });	
  }
  
public:
	stream_server(boost::asio::io_service &io_service,
	           boost::shared_ptr<base_stream> _stream,
			   typename Protocol::endpoint endpoint)
	:acceptor(io_service, endpoint), stream(_stream) {
		async_accept();
	}
};

class serial_stream : public base_stream
{
	boost::asio::serial_port serial;

	uint8_t read_buf[256];	
	
	void read_callback(size_t bytes_transferred, const boost::system::error_code &error) {
		if(error || !bytes_transferred) {
			std::cerr << "read_callback error" << ": " << error << error.message() << ": " << std::endl;
			return;
		}
		
		data_received(read_buf,bytes_transferred);
		initiate_read();
	}
	
	bool check_callback(size_t bytes_transferred, const boost::system::error_code &error) {
		if (error) return true;
		return bytes_transferred > 0;
	}
	
	void initiate_read() {
		namespace ph = boost::asio::placeholders;
		
		boost::asio::async_read(serial,boost::asio::buffer(read_buf),
			boost::bind(&serial_stream::check_callback,this,ph::bytes_transferred,ph::error),
			boost::bind(&serial_stream::read_callback,this,ph::bytes_transferred,ph::error));	
	}
public:
	serial_stream(boost::asio::io_service &io_svc,
		const char *path, const char *opts = "{ \"parity\": 2, \"baud\": 38400 }")
	:serial(io_svc) {
		serial.open(path);
		
		std::istringstream json_src(opts);
		
		boost::property_tree::ptree options;
		boost::property_tree::read_json(json_src,options);
		
		int baud = options.get<int>("baud");
		std::cerr << "baud: " << baud << std::endl;
		int parity = options.get<int>("parity");
		std::cerr << "parity: " << parity << std::endl;
		
		using boost::asio::serial_port_base;		
		
		const serial_port_base::parity parity_opt((serial_port_base::parity::type)parity);
		const serial_port_base::stop_bits stop_bits_opt(serial_port_base::stop_bits::one);
		
		serial.set_option(serial_port_base::baud_rate(baud));
		serial.set_option(serial_port_base::character_size(8));
		serial.set_option(parity_opt);
		serial.set_option(stop_bits_opt);
		serial.set_option(serial_port_base::flow_control(serial_port_base::flow_control::none));
		
		initiate_read();
	}
	
	virtual int send(void *data, size_t len, base_stream::send_callback callback) {
		boost::asio::async_write(serial,boost::asio::buffer(data,len),
			[callback](const boost::system::error_code &error, size_t bytes_transferred) {
				callback(0,bytes_transferred);										
			});
		return 0;
	}
};

class dispatcher
{
	usb::context context;
	cp210x cp;
	
	boost::signals2::connection connection;
	
	std::vector< boost::shared_ptr<base_stream> > streams;
	std::vector< boost::shared_ptr<base_stream> > streams2;
	
	void dispatch(int status, void *data, size_t len) {
		if(!status) {
			streams[0]->data_received(data,len);
		}
	}
public:
	dispatcher(usb::context &_context,boost::asio::io_service &io_svc)
	:context(_context),cp(context,true) {
		if(cp) {
		
			cp.set_baud(38400);
			cp.set_ctl(cp210x::data8 | cp210x::parity_none | cp210x::stop1);
		
			auto sender = [this](void *data, size_t len, base_stream::send_callback cb) {
				return this->cp.send_async(data,len,cb);		
			};
		
			for(uint32_t i = 0; i < 6; i++) {
				streams.push_back(boost::make_shared<data_stream>(sender,i));
			}
		
			connection = cp.data_received.connect([this](int status, void *data, size_t len) {
				return this->dispatch(status,data,len);
			});
		}
		
		streams2.push_back(boost::make_shared<serial_stream>(io_svc,"/dev/ttyUSB0"));
		
	}
	
	~dispatcher() {
		std::cerr << "~dispatcher" << std::endl;
	}

	boost::shared_ptr<base_stream> get_stream(size_t i) {
		return streams[i];
	}		
	
	boost::shared_ptr<base_stream> get_stream2(size_t i) {
		return streams2[i];
	}	
	
	operator bool() const {
		return cp;
	}
};

int main() {
	usb::context context(3);
	if(!context) {
		return 1;
	}
	
	typedef stream_server<boost::asio::ip::tcp> tcp_server;
	typedef stream_server<boost::asio::local::stream_protocol> unix_server;
	
	
	try {
		boost::asio::io_service io_service;
		
		dispatcher d(context,io_service);
		/*if(!d) {
			std::cerr << "No cp210x device found" << std::endl;
			return 2;
		}*/
		
		::unlink("/tmp/stream");
		unix_server server(io_service,d.get_stream2(0),
		                   boost::asio::local::stream_protocol::endpoint("/tmp/stream"));
		 
		//tcp_server server(io_service,d.get_stream2(0),tcp::endpoint(tcp::v4(),1200));
		
		io_service.run();
	} catch (std::exception& e) {
		std::cerr << "Exception:" << e.what() << std::endl;
		return 3;
	}

	return 0;
}