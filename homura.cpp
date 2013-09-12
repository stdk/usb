#include <iostream>
#include <cstdint>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/make_shared.hpp>
#include <vector>
#include <queue>

#include <cp210x.h>

using boost::asio::ip::tcp;

class data_stream
{
	//uint32_t index;
	//uint8_t data[64 - sizeof(uint32_t)];
	uint8_t data[64];
	
public:
	typedef boost::function<int (void *data, size_t len)> sender_t;

	data_stream(sender_t _sender, uint32_t _index):sender(_sender) {
	
	}
	
	~data_stream() {
		std::cerr << "~data_stream" << std::endl;
	}
	
	auto buffer() -> decltype(boost::asio::buffer(data)) {
		return boost::asio::buffer(data);
	}
	
	int send(size_t len) {
		return sender(data,len);
	}
	
	boost::signals2::signal<void (void *data, size_t len)> data_received;

private:
	sender_t sender;
};

class tcp_connection
	: public boost::enable_shared_from_this<tcp_connection>
{

	tcp_connection(boost::asio::io_service& io_service)
	: socket(io_service) {
		
	}

	tcp::socket socket;
	tcp::endpoint endpoint;
	
	boost::signals2::connection connection;	
public:
	typedef boost::shared_ptr<tcp_connection> pointer;

	~tcp_connection() {
		std::cerr << "~tcp_connection" << std::endl;
	}

	static pointer create(boost::asio::io_service& io_service) {
		return pointer(new tcp_connection(io_service));
	}

	tcp::socket& get_socket() {
		return socket;
	}
  
	tcp::endpoint& get_endpoint() {
		return endpoint;  
	}  

	void net_to_device(boost::shared_ptr<data_stream> stream) {
		pointer shared = shared_from_this();
		boost::asio::async_read(socket,stream->buffer(),
			//check callback
			[](const boost::system::error_code &error,
			   size_t bytes_transferred) {
				std::cerr << "check: " << bytes_transferred << std::endl;
				return !!error || bytes_transferred > 1;
			},
			//read callback
			[shared,stream](const boost::system::error_code &error,
			     size_t bytes_transferred) {
				 std::cerr << "sender: "
				      << error.message() <<" : "
				      << bytes_transferred << std::endl;
				 
				if(!error) {
					stream->send(bytes_transferred);
					shared->net_to_device(stream);
				} else {
					shared->connection.disconnect();
				}					
			});
	}

	void start(boost::shared_ptr<data_stream> stream) {
		net_to_device(stream);
		
		pointer shared = shared_from_this();		
		auto receiver = [shared](void *data, size_t len) {
			auto bytes = usb::format_bytes(data,len);
			std::cerr << "receiver: " << bytes.get() << std::endl;
			
			auto c = shared->connection;
			boost::asio::async_write(shared->socket,boost::asio::buffer(data,len),
				[c](const boost::system::error_code &error,
				    size_t bytes_transferred) {
					
					std::cerr << "async_write handler: " 
					          << error.message() << " : "
							  << bytes_transferred << std::endl;
					
					if(error) {
						c.disconnect();
					}
				});
			};
		
		connection = stream->data_received.connect(receiver);
	}
};

class tcp_server
{
	tcp::acceptor acceptor;	
	boost::shared_ptr<data_stream> stream;
	
	void async_accept()
	{
		tcp_connection::pointer connection =
		 tcp_connection::create(acceptor.get_io_service());

		acceptor.async_accept(connection->get_socket(),
		                      connection->get_endpoint(),
		 [=,this](const boost::system::error_code &error) {
			std::cerr << "accept_handler: " 
			          << error.message()
			          << " | endpoint: " 
			          << connection->get_endpoint() << std::endl;
			
			if(!error) {
				connection->start(this->stream);
				this->async_accept();
			}
		 });	
  }
  
public:
	tcp_server(boost::asio::io_service &io_service,
	           boost::shared_ptr<data_stream> _stream,
			   uint16_t port)
	:acceptor(io_service, tcp::endpoint(tcp::v4(), port)),stream(_stream) {
		async_accept();
	}
};



class dispatcher
{
	usb::context context;
	cp210x cp;
	
	boost::signals2::connection connection;
	
	std::vector< boost::shared_ptr<data_stream> > streams;
	
	//std::queue< std::pair<void*,size_t> > send_queue;
	
	void dispatch(int status, void *data, size_t len) {
		//fprintf(stderr,"dispatch[%i][%p][%i]\n",status,data,len);
		if(!status) {
			streams[0]->data_received(data,len);
		}
	}
public:
	dispatcher(usb::context &_context):context(_context),cp(context,true) {
		if(!cp) return;
		
		cp.set_baud(38400);
		cp.set_ctl(cp210x::data8 | cp210x::parity_none | cp210x::stop1);
		
		auto sender = [this](void *data, size_t len) -> int {
			return this->cp.send_async(data,len,[](int status, size_t len) {
				fprintf(stderr,"sender completed[%i][%i]\n",status,len);
			});		
			//fprintf(stderr,"sender[%p][%i]\n",data,len);
			/*this->send_queue.push( std::make_pair(data,len) );
			
			/*std::cerr << "Send queue:" << std::endl;
			for(auto i = this->send_queue.begin(); i != this->send_queue.end(); i++) {
				fprintf(stderr,"[%p][%i]\n",i->first,i->second);	
			}*/			
			
			return 0;
		};
		
		for(uint32_t i = 0; i < 6; i++) {
			streams.push_back(boost::make_shared<data_stream>(sender,i));
		}
		
		connection = cp.data_received.connect([this](int status, void *data, size_t len) {
			return this->dispatch(status,data,len);
		});
	}
	
	~dispatcher() {
		std::cerr << "~dispatcher" << std::endl;
	}

	boost::shared_ptr<data_stream> get_stream(size_t i) {
		return streams[i];
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
	
	dispatcher d(context);
	if(!d) {
		std::cerr << "No cp210x device found" << std::endl;
		return 2;
	}
	
	try {
		boost::asio::io_service io_service;
		tcp_server server(io_service,d.get_stream(0),1200);
		io_service.run();
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 3;
	}

	return 0;
}