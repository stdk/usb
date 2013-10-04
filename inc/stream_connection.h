#ifndef STREAM_CONNECTION_H
#define STREAM_CONNECTION_H

#include <base_stream.h>
#include <usb.h>

#include <boost/enable_shared_from_this.hpp>
#include <boost/signals2.hpp>
#include <boost/asio.hpp>
#include <stdlib.h>
#include <cstdint>

template<class Protocol>
class stream_connection 
:public boost::enable_shared_from_this< stream_connection<Protocol> >
{
	bool debug;
	bool async_write;
	uint8_t read_buf[256];
	
	typename Protocol::socket socket;
	typename Protocol::endpoint endpoint;
	
	boost::signals2::connection connection;	

	stream_connection(boost::asio::io_service& io_service)
	:debug(getenv("DEBUG_STREAM_CONNECTION") != 0),
	 async_write(getenv("STREAM_CONNECTION_ASYNC_WRITE") != 0),
	 socket(io_service) {
		
	}
	
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
				return !!error || bytes_transferred > 1;
			},
			//read callback
			[=](const boost::system::error_code &error,
				size_t bytes_transferred) {
				if(shared->debug) {
					auto bytes = usb::format_bytes(shared->read_buf,bytes_transferred);
					std::cerr << "recv[C][" << bytes_transferred << "] " 
					          << bytes.get() << " | " << error.message() << std::endl;
					
				}
				 
				if(!error) {
					stream->send(read_buf,bytes_transferred,[=](int status, size_t len) {
						if(shared->debug) {
							std::cerr << "sent[S][" << len << "][" 
							          << status << "]" << std::endl;
						}
						shared->net_to_device(stream);					
					});					
				} else {
					shared->connection.disconnect();
					shared->disconnected();
					//shared.reset();
				}					
			});
	}

	void start(boost::shared_ptr<base_stream> stream) {
		net_to_device(stream);
		
		pointer shared = this->shared_from_this();		
		auto receiver = [shared](void *data, size_t len) {
			if(shared->debug) {
				auto bytes = usb::format_bytes(data,len);
				std::cerr << "recv[S][" << len << "] " << bytes.get() << std::endl;
			}

			if(shared->async_write) {
				uint8_t *tmp = new uint8_t[len];
				uint8_t *i = tmp;
				std::copy((uint8_t*)data, (uint8_t*)data+len, i);
			
				boost::asio::async_write(shared->socket,boost::asio::buffer(tmp,len/*data,len*/),
				[shared,tmp](const boost::system::error_code &error,
				    size_t bytes_transferred) {
					
					if(shared->debug) {
						std::cerr << "sent[C][" << bytes_transferred << "] "
						          << error.message() << std::endl;
					}
					
					if(error) {
						shared->connection.disconnect();
					}
					
					delete[] tmp;
				});
			} else {
				boost::system::error_code error;
				boost::asio::write(shared->socket,boost::asio::buffer(data,len),error);
			
				if(shared->debug) {
					std::cerr << "sent[C][" << len << "] "
						      << error.message() << std::endl;			
				}
			
				if(error) {
					shared->connection.disconnect();			
				}
			}
		};
		
		connection = stream->data_received.connect(receiver);
	}
};

#endif