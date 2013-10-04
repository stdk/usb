#ifndef BASE_STREAM_H
#define BASE_STREAM_H

#include <iostream>
#include <boost/function.hpp>
#include <boost/signals2.hpp>

class base_stream
{
public:
	typedef boost::function<void (int status, size_t len)> send_callback;

	~base_stream() {
		std::cerr << "~base_stream" << std::endl;	
	}

	boost::signals2::signal<void (void *data, size_t len)> data_received;
	virtual int send(void *data, size_t len, send_callback callback) = 0;
};

#endif