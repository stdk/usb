#include <dispatcher.h>

#include <cp210x.h>
#include <cp210x_stream.h>

#include <iostream>
#include <boost/make_shared.hpp>

dispatcher::dispatcher(usb::context &_context,boost::asio::io_service &io_svc)
:context(_context),cp(context,true) {
	if(!cp) return;
	
//	cp.set_baud(38400);
//	cp.set_ctl(cp210x::data8 | cp210x::parity_even | cp210x::stop1);
	
//	uint32_t baud = cp.get_baud();
//	uint16_t ctl = cp.get_ctl();
		
	auto sender = [this](void *data, size_t len, base_stream::send_callback cb) {
		return this->cp.send_async(data,len,cb);		
	};
		
	for(uint32_t i = 0; i < 6; i++) {
		streams.push_back(boost::make_shared<cp210x_stream>(sender,i));
	}
		
	connection = cp.data_received.connect([this](int status, void *data, size_t len) {
		return this->dispatch(status,data,len);
	});
}

dispatcher::~dispatcher() {
	std::cerr << "~dispatcher" << std::endl;
	connection.disconnect();
}

void dispatcher::dispatch(int status, void *data, size_t len) {
	if(!status) {
		streams[0]->data_received(data,len);
	}
}

boost::shared_ptr<base_stream> dispatcher::get_stream(size_t i) {
	return streams[i];
}

dispatcher::operator bool() const {
	return cp;
}