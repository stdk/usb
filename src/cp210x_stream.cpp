#include <cp210x_stream.h>

#include <iostream>

cp210x_stream::cp210x_stream(sender_t _sender, uint32_t _index):sender(_sender) {
	
}

cp210x_stream::~cp210x_stream() {
	std::cerr << "~data_stream" << std::endl;
}

int cp210x_stream::send(void *data, size_t len, base_stream::send_callback callback) {
	return sender(data,len,callback);
}