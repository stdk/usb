#include <cstdint>
#include <stdlib.h>
#include <signal.h>
#include <boost/asio.hpp>
#include <boost/make_shared.hpp>

#include <stream_server.h>
#include <serial_dmx.h>

int main() {
	typedef stream_server<boost::asio::ip::tcp> tcp_server;
	typedef stream_server<boost::asio::local::stream_protocol> unix_server;
	
	using boost::asio::local::stream_protocol;
		
	boost::asio::io_service io_service;
	
	const char *bar_socket = "/tmp/bar";
	::unlink(bar_socket);
	
	const char *screen_socket = "/tmp/screen";
	::unlink(screen_socket);
	
	boost::asio::signal_set signals(io_service, SIGINT, SIGQUIT);
	signals.async_wait([&](const boost::system::error_code &error, int s) {
		if(!error) {
			std::cerr << "signal: " << s << std::endl;
			
			io_service.stop();
		}
	});

	try {
		serial_dmx d(io_service);
	
	/*    auto serial1 = boost::make_shared<serial_stream>(io_service,"/dev/ttyUSB0");
	
		::unlink("/tmp/stream");
		unix_server s1(io_service,serial1,
		               boost::asio::local::stream_protocol::endpoint("/tmp/stream"));*/
		
		unix_server bar_server(io_service,d.get_stream(0),
		                boost::asio::local::stream_protocol::endpoint(bar_socket));
		 
		unix_server screen_server(io_service,d.get_stream(1),
		                boost::asio::local::stream_protocol::endpoint(screen_socket));
						
		tcp_server server2(io_service,d.get_stream(2),
		                boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),1301));
		
		tcp_server server3(io_service,d.get_stream(3),
		                boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),1303));
		
		tcp_server server4(io_service,d.get_stream(4),
		                boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),1304));
						
		tcp_server server5(io_service,d.get_stream(5),
		                boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),1305));
		
		io_service.run();
		
		::unlink(bar_socket);
		::unlink(screen_socket);
	} catch (std::exception& e) {
		std::cerr << "Exception:" << e.what() << std::endl;
		return 3;
	}

	return 0;
}
