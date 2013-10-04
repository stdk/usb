#ifndef SERIAL_STREAM_H
#define SERIAL_STREAM_H

#include <boost/asio.hpp>
#include <cstdint>

#include <base_stream.h>

class serial_stream : public base_stream
{
	boost::asio::serial_port serial;

	uint8_t read_buf[256];	
	
	void read_callback(size_t bytes_transferred, const boost::system::error_code &error);
	bool check_callback(size_t bytes_transferred, const boost::system::error_code &error);
	
	void initiate_read();
public:
	serial_stream(boost::asio::io_service &io_svc,
		const char *path, const char *opts = "{ \"parity\": 2, \"baud\": 38400 }");
	
	virtual int send(void *data, size_t len, base_stream::send_callback callback);
};

#endif