#ifndef CP210X_H
#define CP210X_H

#include <usb.h>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>

class cp210x 
{
	usb::context context;
	usb::device device;
	usb::device_handle handle;
	usb::configuration conf;
	
	uint16_t interface_number;
	usb::interface iface;
	
	usb::transfer recv_transfer;

	int completed;
	boost::thread io_thread;
	
	void io_thread_func();
	
	bool auto_recv;
	
	static usb::device find_device(const usb::context &context, uint16_t vid, uint16_t pid);

    int set_device_config(uint16_t value, uint16_t index,const void *data, size_t len);
	int get_device_config(uint16_t value, uint16_t index,void *data, size_t len);
	
	int set_interface_config_single(uint8_t request, unsigned int data);
	int set_interface_config(uint8_t request, const void *data, size_t len);
	int get_interface_config(uint8_t request, void *data, size_t len);
	
	int set_config_string(uint16_t value, size_t max_length, char* data);
public:
	enum Data {
		data_mask  = 0x0f00,
		data5 = 0x0500,
		data6 = 0x0600,
		data7 = 0x0700,
		data8 = 0x0800,
		data9 = 0x0900
	};
	
	enum Parity {
		parity_mask  = 0x00f0,
		parity_none  = 0x0000,
		parity_odd   = 0x0010,
		parity_even  = 0x0020,
		parity_mark  = 0x0030,
		parity_space = 0x0040		
	};
	
	enum Stop {
		stop_mask = 0x000f,
		stop1     = 0x0000,
		stop1_5   = 0x0001,
		stop2     = 0x0002
	};
		
public:
	cp210x(const usb::context &ctx, bool auto_recv = false);
	~cp210x();

	uint32_t get_baud();
	int set_baud(uint32_t baud);
	
	uint16_t get_ctl();
	int set_ctl(uint16_t ctl);

	boost::signals2::signal<void (int status, void *data, size_t len)> data_received;
	
	int recv_async();
	int send_async(void *buffer, size_t len, 
	               boost::function<void (int status, size_t bytes_transferred)> callback,
	               uint32_t timeout = 1000);
	
	int send(void *buffer, size_t len, uint32_t timeout = 1000);
	int recv(void *buffer, size_t len, uint32_t timeout = 1000);
		
	int set_product_string(char *s);
	int get_product_string(char *s, size_t len);
	
	operator bool() const;	
};

#endif // CP210X_H