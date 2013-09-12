#ifndef USB_H
#define USB_H

#include <libusb-1.0/libusb.h>
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>
#include <boost/function.hpp>
#include <boost/signals2.hpp>

namespace usb {

boost::shared_array<char> format_bytes(const void *data, size_t len);

/* ------------------------------------ */

const char* error_name(int error_code);

class context
{
	boost::shared_ptr<libusb_context> ctx;
public:
	context(int debug_level);
	
	int handle_events_timeout(struct timeval *tv);
	
	operator libusb_context*() const;
	operator bool() const;
};

/* ------------------------------------ */

class device_descriptor : public libusb_device_descriptor
{
	int ret;
public:
	device_descriptor(libusb_device *dev);
	
	void print(const char *prefix = "");
	
	operator bool() const;
};

/* ------------------------------------ */

class config_descriptor
{
	boost::shared_ptr<libusb_config_descriptor> desc;
public:
	config_descriptor(libusb_device *device);

	libusb_config_descriptor* operator->() const;
	
	operator bool() const;
};

/* ------------------------------------ */

class device
{
	libusb_device *dev;
public:
	device(libusb_device *_dev);
	device(const device &d);
	device(device &&d);
	device& operator=(const device &d);
	~device();	

	device_descriptor descriptor() const;
	operator libusb_device*() const;
	operator bool() const;
};

/* ------------------------------------ */

class device_handle
{
	boost::shared_ptr<libusb_device_handle> handle;
	static void deleter(libusb_device_handle *handle);
public:
	device_handle(device &dev, bool reset = false);
	device_handle(const context &ctx,uint16_t vendor_id, uint16_t product_id);

	int set_configuration(int config_number);
	int configuration() const;

	int claim_interface(int interface_number);
	int release_interface(int interface_number);
	
	int control_transfer(uint8_t bmRequestType,
	                     uint8_t bRequest,
						 uint16_t wValue,
						 uint16_t wIndex,
						 const void *data,
						 uint16_t wLength,
						 unsigned int timeout);
	
	int bulk_transfer(uint8_t endpoint,
	                  void *data,
					  int length,
					  int *transferred,
					  unsigned int timeout);
	
	int interrupt_transfer(uint8_t endpoint,
                           uint8_t *data,
						   int length,
						   int *transferred,
						   unsigned int timeout);
		
	operator bool() const;
	operator libusb_device_handle*() const;
};

/* ------------------------------------ */

class interface
{
	device_handle handle;
	int interface_number;
	int ret;
public:
	interface(device_handle _handle,int _interface_number);
	~interface();
	
	operator bool() const;
};

/* ------------------------------------ */

class configuration
{
	device_handle handle;
	int config_number;
	int ret;
public:
	configuration(device_handle _handle, int _config_number);
	~configuration();
	
	operator bool() const;
};

/* ------------------------------------ */

class device_list
{
	ssize_t count;
	libusb_device **list;
public:
	device_list(const context &ctx);
	~device_list();

	device find(uint16_t vid, uint16_t pid);
	ssize_t get_count() const;
};

/* ------------------------------------ */

class transfer
{
	static void generic_callback(libusb_transfer *transfer);
	
	void init_native_transfer();
	
	device_handle dev;
	boost::shared_ptr<libusb_transfer> tr;
	boost::shared_array<uint8_t> data_buffer;

public:
	transfer(device_handle _dev,
	         uint8_t type = 0,
	         uint8_t endpoint = 0,			 
			 uint8_t flags = 0,
			 unsigned int timeout = 500);
	
	~transfer();
	
	/* ------------------------------ */
	
	int submit();
	int cancel();
	
	/* ------------------------------ */
	
	boost::signals2::signal<void (transfer *tr)> transfer_completed;
	
	/* ------------------------------ */	
	
	void fill_control(uint8_t bmRequestType,
	                  uint8_t bRequest,
				      uint16_t wValue,
				      uint16_t wIndex,
				      const void *data,
                      uint16_t wLength);
	
	/* ------------------------------ */
	
	boost::shared_array<uint8_t> buffer();
	void set_buffer(boost::shared_array<uint8_t> buffer,size_t len);	
	void set_buffer(void *buffer, size_t len);
	void allocate_buffer(size_t len);
	
	uint8_t flags() const;
	void set_flags(uint8_t flags);
	
	uint8_t endpoint() const;
	void set_endpoint(uint8_t endpoint);
	
	uint8_t type() const;
	void set_type(uint8_t type);
	
	int status() const;
	const char* status_str() const;
	
	unsigned int timeout() const;
	void set_timeout(unsigned int timeout);
	
	int length() const;
	int actual_length() const;	
	
	/* ------------------------------ */
	
	operator bool() const;
};

} //namespace usb

#endif //USB_H