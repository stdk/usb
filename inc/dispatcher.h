#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <cp210x.h>
#include <base_stream.h>

#include <vector>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <boost/shared_ptr.hpp>

class dispatcher
{
	usb::context context;
	cp210x cp;
	
	boost::signals2::connection connection;
	
	std::vector< boost::shared_ptr<base_stream> > streams;
	
	void dispatch(int status, void *data, size_t len);
public:
	dispatcher(usb::context &_context,boost::asio::io_service &io_svc);
	~dispatcher();

	boost::shared_ptr<base_stream> get_stream(size_t i);	
	
	operator bool() const;
};

#endif