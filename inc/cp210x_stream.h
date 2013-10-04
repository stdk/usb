#ifndef CP210X_STREAM
#define CP210X_STREAM

#include <boost/function.hpp>

#include <base_stream.h>

class cp210x_stream : public base_stream
{
public:
	typedef boost::function<int (void *data, size_t len, send_callback callback)> sender_t;

	cp210x_stream(sender_t _sender, uint32_t _index);
	~cp210x_stream();
	
	virtual int send(void *data, size_t len, base_stream::send_callback callback);
private:
	sender_t sender;
};

#endif