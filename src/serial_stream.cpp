#include <serial_stream.h>

#include <iostream>
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/bind.hpp>

serial_stream::serial_stream(boost::asio::io_service &io_svc, 
                             const char *path, const char *opts):serial(io_svc) {
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

void serial_stream::read_callback(size_t bytes_transferred, const boost::system::error_code &error) {
	if(error || !bytes_transferred) {
		std::cerr << "read_callback error" << ": " << error << error.message() << ": " << std::endl;
		return;
	}
		
	data_received(read_buf,bytes_transferred);
	initiate_read();
}

bool serial_stream::check_callback(size_t bytes_transferred, const boost::system::error_code &error) {
	if (error) return true;
	return bytes_transferred > 0;
}

void serial_stream::initiate_read() {
	namespace ph = boost::asio::placeholders;
		
	boost::asio::async_read(serial,boost::asio::buffer(read_buf),
		boost::bind(&serial_stream::check_callback,this,ph::bytes_transferred,ph::error),
		boost::bind(&serial_stream::read_callback,this,ph::bytes_transferred,ph::error));	
}

int serial_stream::send(void *data, size_t len, base_stream::send_callback callback) {
	boost::asio::async_write(serial,boost::asio::buffer(data,len),
		[callback](const boost::system::error_code &error, size_t bytes_transferred) {
			callback(0,bytes_transferred);										
		});
	return 0;
}