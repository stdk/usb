#include <iostream>
#include <cstdint>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

#include <cp210x.h>

using boost::asio::ip::tcp;

class tcp_connection
	: public boost::enable_shared_from_this<tcp_connection>
{

	tcp_connection(boost::asio::io_service& io_service)
	: socket(io_service) {
		
	}

	tcp::socket socket;
	tcp::endpoint endpoint;
	uint8_t read_buf[64];
	
	boost::signals2::connection c;
	
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

	void net_to_device(cp210x *cp) {
		pointer shared = shared_from_this();
		boost::asio::async_read(socket,boost::asio::buffer(read_buf),
			//check callback
			[](const boost::system::error_code &error,
			   size_t bytes_transferred) {
				std::cerr << "check: " << bytes_transferred << std::endl;
				return !!error || bytes_transferred > 1;
			},
			//read callback
			[shared,cp](const boost::system::error_code &error,
			     size_t bytes_transferred) {
				 std::cerr << "sender: "
				      << error.message() <<" : "
				      << bytes_transferred << std::endl;
				 
				if(!error) {
					cp->send_async(shared->read_buf,bytes_transferred);
					shared->net_to_device(cp);
				} else {
					shared->c.disconnect();
				}					
			});
	}

	void start(cp210x *cp) {
		net_to_device(cp);
		
		pointer shared = shared_from_this();		
		auto receiver = [shared](boost::signals2::connection c,
		                            int status, void *data, size_t len) {
			std::cerr << "receiver: " << status  << " : " << len << std::endl;
			
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
		
		c = cp->data_received.connect_extended(receiver);
	}
};


class tcp_server
{
	tcp::acceptor acceptor;	
	cp210x *cp;
	
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
				connection->start(this->cp);
				this->async_accept();
			}
		 });	
  }
  
public:
	tcp_server(boost::asio::io_service &io_service, cp210x *_cp)
	:acceptor(io_service, tcp::endpoint(tcp::v4(), 13)),
	 cp(_cp) {
		async_accept();
	}
};

int main() {
	usb::context context(3);
	if(!context) {
		return 1;
	}
	
	cp210x cp(context,true);
	if(!cp) {
		std::cerr << "No cp210x device found" << std::endl;
		return 2;
	}
	
	cp.set_baud(38400);
    cp.set_ctl(cp210x::data8 | cp210x::parity_none | cp210x::stop1);

	try {
		boost::asio::io_service io_service;
		tcp_server server(io_service,&cp);
		io_service.run();
	} catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 3;
	}

	return 0;
}