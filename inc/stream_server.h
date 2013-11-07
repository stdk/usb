#ifndef STREAM_SERVER_H
#define STREAM_SERVER_H

#include <stream_connection.h>
#include <boost/shared_array.hpp>
#include <boost/asio.hpp>

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

#endif