#include <cstdint>
#include <stdlib.h>
#include <boost/asio.hpp>
#include <boost/make_shared.hpp>

#include <stream_server.h>
#include <dispatcher.h>
#include <serial_stream.h>
#include <usb.h>

static const int log_level = getenv("DEBUG_HOMURA") != 0;

int main() {
	usb::context context(3);
	if(!context) {
		return 1;
	}
	
	typedef stream_server<boost::asio::ip::tcp> tcp_server;
	typedef stream_server<boost::asio::local::stream_protocol> unix_server;
	
	using boost::asio::local::stream_protocol;
		
	boost::asio::io_service io_service;
		
	dispatcher d(context,io_service);
	if(!d) {
		std::cerr << "No device found" << std::endl;
		return 2;
	}
	
	try {
	/*    auto serial1 = boost::make_shared<serial_stream>(io_service,"/dev/ttyUSB0");
	
		::unlink("/tmp/stream");
		unix_server s1(io_service,serial1,
		               boost::asio::local::stream_protocol::endpoint("/tmp/stream"));*/
					   
		/*::unlink("/tmp/cp");
		unix_server s2(io_service,d.get_stream(0),
		               boost::asio::local::stream_protocol::endpoint("/tmp/cp"));*/
		 
		tcp_server server(io_service,d.get_stream(0),
		                boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(),1200));
		
		io_service.run();
	} catch (std::exception& e) {
		std::cerr << "Exception:" << e.what() << std::endl;
		return 3;
	}

	return 0;
}
