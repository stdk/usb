#include <serial_dmx.h>

#include <iostream>
#include <boost/make_shared.hpp>

dmx_stream::dmx_stream(sender_t _sender, uint32_t _index):sender(_sender) {
	
}

dmx_stream::~dmx_stream() {
	std::cerr << "~dmx_stream" << std::endl;
}

int dmx_stream::send(void *data, size_t len, base_stream::send_callback callback) {
	return sender(data,len,callback);
}

serial_dmx::serial_dmx(boost::asio::io_service &io_svc):serial(io_svc,"/dev/ttyUSB1") {
	
	auto sender = [this](void *data, size_t len, base_stream::send_callback cb) {
		return this->serial.send(data,len,cb);	
	};
		
	for(uint32_t i = 0; i < 6; i++) {
		streams.push_back(boost::make_shared<dmx_stream>(sender,i));
	}
		
	connection = serial.data_received.connect([this](void *data, size_t len) {
		return this->dispatch(data,len);
	});
}

serial_dmx::~serial_dmx() {
	std::cerr << "~serial_dmx" << std::endl;
	connection.disconnect();
}

void serial_dmx::dispatch(void *data, size_t len) {
	while(len != 0) {

		struct header_t {
			uint8_t addr;
			uint8_t len;
			uint8_t data[0];
		} __attribute__((packed)) *header = (header_t*)data;
	
		if(len < sizeof(header_t)) {
			std::cerr << "incorrect packet received with len " << len << std::endl;
			return;
		}
	
		std::cerr << header->addr << " " << header->len << std::endl;
	
		if(header->addr >= streams.size()) {
			std::cerr << "incorrect addr = " << header->addr << std::endl;
			return;
		}
	
		if(header->len + sizeof(header_t) > len) {
			std::cerr << "incorrect header len = " << header->len << " with full len = " << len << std::endl;
			return;
		}

		streams[header->addr]->data_received(header->data, header->len);
		
		uint8_t *d = (uint8_t*)data;
		size_t diff = sizeof(header_t) + header->len;
		d += diff;
		len -= diff;
	}
}

boost::shared_ptr<base_stream> serial_dmx::get_stream(size_t i) {
	return streams[i];
}

serial_dmx::operator bool() const {
	return true;
}