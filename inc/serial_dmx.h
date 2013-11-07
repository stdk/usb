#ifndef SERIAL_DMX_H
#define SERIAL_DMX_H

#include <base_stream.h>

#include <vector>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <boost/shared_ptr.hpp>
#include <serial_stream.h>

class dmx_stream : public base_stream
{
public:
	typedef boost::function<int (void *data, size_t len, send_callback callback)> sender_t;

	dmx_stream(sender_t _sender, uint32_t _index);
	~dmx_stream();
	
	struct buffer_t {
		uint8_t addr;
		uint8_t len;
		uint8_t data[62];	
	} buffer;
	
	virtual int send(void *data, size_t len, base_stream::send_callback callback);
private:
	sender_t sender;
};

class serial_dmx
{
	boost::signals2::connection connection;
	serial_stream serial;
	
	std::vector< boost::shared_ptr<base_stream> > streams;
	
	void dispatch(void *data, size_t len);
public:
	serial_dmx(boost::asio::io_service &io_svc);
	~serial_dmx();

	boost::shared_ptr<base_stream> get_stream(size_t i);	
	
	operator bool() const;
};

#endif //SERIAL_DMX_H