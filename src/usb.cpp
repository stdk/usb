#include <usb.h>

using namespace usb;

const int log_level = (getenv("DEBUG_USB") != 0);

/* ------------------------------------ */	

boost::shared_array<char> usb::format_bytes(const void *data, size_t len)
{
	boost::shared_array<char> buffer(new char[len ? len*3 : 1]);
	buffer[0] = 0;
	
	char    *b = buffer.get();
	uint8_t *d = (uint8_t*)data;
	while(len--) b += snprintf(b,len ? 4 : 3,"%02hhX ",*d++);
	
	return buffer;
}

/* ------------------------------------ */	

const char* error_name(int error_code) {
	return libusb_error_name(error_code);
}

/* ------------------------------------ */	

context::context(int debug_level) {
	fprintf(stderr,"libusb_init\n");
	
	libusb_context *_ctx;
	int ret = libusb_init(&_ctx);
	if(ret) {
		fprintf(stderr,"libusb_init failed with [%i]\n",ret);	
		return;
	}	
	
	auto deleter = [](libusb_context *ctx) {
		fprintf(stderr,"libusb_exit[%p]\n",ctx);
		libusb_exit(ctx);	
	};
	ctx = boost::shared_ptr<libusb_context>(_ctx,deleter);
	
	libusb_set_debug(ctx.get(),debug_level);
}


context::operator libusb_context*() const {
	return ctx.get();
}

context::operator bool() const {
	return ctx;
}

int context::handle_events_timeout(struct timeval *tv) {
	return libusb_handle_events_timeout(ctx.get(),tv);
}

/* ------------------------------------ */

int device_handle::claim_interface(int interface_number) {
	return handle ? libusb_claim_interface(handle.get(),interface_number) : -1;
}

int device_handle::release_interface(int interface_number) {
	return handle ? libusb_release_interface(handle.get(),interface_number) : -1;
}

int device_handle::set_configuration(int config_number) {
	return handle ? libusb_set_configuration(handle.get(),config_number) : -1;
}

int device_handle::configuration() const {
	if(!handle) return -1;
	int num = -1;
	return libusb_get_configuration(handle.get(),&num)==0 ? num : -1;	
}

void device_handle::deleter(libusb_device_handle *handle) {
	if(handle) {
		fprintf(stderr,"libusb_close[%p]\n",handle);
		libusb_close(handle);
	}
}

device_handle::device_handle(device &dev, bool reset) {
	if(!dev) {
		fprintf(stderr,"There is no device to open.\n");
		return;
	}
	
	libusb_device_handle *_handle = 0;
	int ret = libusb_open(dev,&_handle);
	if(ret) {
		fprintf(stderr,"Unable to open usb device\n");
		return;
	}
	
	if(reset) {
		int ret = libusb_reset_device(_handle);
		if(ret) {
			fprintf(stderr,"libusb_reset_device(%p) -> %s\n",_handle,libusb_error_name(ret));
		}
	}
	
	handle = boost::shared_ptr<libusb_device_handle>(_handle,deleter);	
	
	if(libusb_kernel_driver_active(handle.get(),0)) {
		fprintf(stderr,"Kernel driver active on usb device. Detaching...\n");
		libusb_detach_kernel_driver(handle.get(),0);
	}	
}

device_handle::device_handle(const context &ctx,uint16_t vendor_id, uint16_t product_id)
:handle(libusb_open_device_with_vid_pid(ctx,vendor_id,product_id),deleter)
{
		
}

device_handle::operator bool() const {
	return handle;
}

device_handle::operator libusb_device_handle*() const {
	return handle.get();
}

int device_handle::control_transfer(uint8_t bmRequestType,
	                     uint8_t bRequest,
						 uint16_t wValue,
						 uint16_t wIndex,
						 const void *data,
						 uint16_t wLength,
						 unsigned int timeout)
{
	int ret = libusb_control_transfer(handle.get(),bmRequestType,
	                                               bRequest,
												   wValue,
												   wIndex,
												   (unsigned char*)data,
												   wLength,
												   timeout);
											
	if(log_level) {	   
		auto bytes = format_bytes(data,wLength);
	
		printf("C(0x%hhX,0x%hhX,0x%hX,0x%hX,[%s],%hi,%i)=%i(%s)\n",
		 bmRequestType,bRequest,wValue,wIndex,bytes.get(),wLength,timeout,ret,
		 ret < 0 ? libusb_error_name(ret) : "");
	}
	
	return ret;
}



int device_handle::bulk_transfer(uint8_t endpoint,
	                             void *data,
					             int length,
								 int *transferred,
								 unsigned int timeout)
{
	int ret = libusb_bulk_transfer(handle.get(),endpoint,
	                                         (unsigned char*)data,
											 length,
											 transferred,
											 timeout);
											 
	/*auto bytes = format_bytes(data,length);
	
	printf("B(0x%hhX,[%s],%i,%i,%i)=%i(%s)\n",
	 endpoint,bytes.get(),length,*transferred,timeout,ret,
	 ret < 0 ? libusb_error_name(ret) : "");*/
	return ret;						 
}

int device_handle::interrupt_transfer(uint8_t endpoint,
                                      uint8_t *data,
						              int length,
						              int *transferred,
						              unsigned int timeout)
{
	return libusb_interrupt_transfer(handle.get(),endpoint,
	                                              data,
												  length,
												  transferred,
												  timeout);
}

/* ------------------------------------ */

device_descriptor::device_descriptor(libusb_device *device) {
	ret = libusb_get_device_descriptor(device, this);
}

void device_descriptor::print(const char *prefix) {
	printf("%sbLength            = 0x%hhX\n",prefix,bLength);
	printf("%sbDescriptorType    = 0x%hhX\n",prefix,bDescriptorType);
	printf("%sbcdUSB             = 0x%hX\n",prefix,bcdUSB);
	printf("%sbDeviceClass       = 0x%hhX\n",prefix,bDeviceClass);
	printf("%sbDeviceSubClass    = 0x%hhX\n",prefix,bDeviceSubClass);
	printf("%sbDeviceProtocol    = 0x%hhX\n",prefix,bDeviceProtocol);
	printf("%sbMaxPacketSize0    = 0x%hhX\n",prefix,bMaxPacketSize0);
	printf("%sidVendor           = 0x%hX\n",prefix,idVendor);
	printf("%sidProduct          = 0x%hX\n",prefix,idProduct);
	printf("%sbcdDevice          = 0x%hX\n",prefix,bcdDevice);
	printf("%siManufacturer      = 0x%hhX\n",prefix,iManufacturer);
	printf("%siProduct           = 0x%hhX\n",prefix,iProduct);
	printf("%siSerialNumber      = 0x%hhX\n",prefix,iSerialNumber);
	printf("%sbNumConfigurations = 0x%hhX\n",prefix,bNumConfigurations);
}

device_descriptor::operator bool() const {
	return ret == 0;
}

/* ------------------------------------ */

config_descriptor::config_descriptor(libusb_device *dev) {

	libusb_config_descriptor *_desc;
	int ret = libusb_get_active_config_descriptor(dev,&_desc);
	if(ret < 0) {
		printf("libusb_get_active_config_descriptor -> %s\n",libusb_error_name(ret));
		return;
	}

	auto deleter = [](libusb_config_descriptor *desc) {
		printf("libusb_free_config_descriptor[%p]\n",desc);	
		libusb_free_config_descriptor(desc);
	};
	
	desc = boost::shared_ptr<libusb_config_descriptor>(_desc,deleter);
}

libusb_config_descriptor* config_descriptor::operator->() const {
	return desc.get();
}

config_descriptor::operator bool() const {
	return desc;
}

/* ------------------------------------ */

device::device(libusb_device *_dev):dev(_dev) {
	printf("device::device[%p]\n",_dev	);
	if(dev) {
		printf("libusb_ref_device(new)\n");
		libusb_ref_device(dev);
	}
}

device::device(const device &d) {
	printf("libusb_ref_device(copy)\n");
	dev = d.dev;
	libusb_ref_device(dev);
}

device::device(device &&d) {
	printf("device move\n");
	dev = d.dev;
	d.dev = nullptr;
}

device& device::operator=(const device &d) {
	printf("device::operator=\n");
	dev = d.dev;
	libusb_ref_device(dev);
}

device::~device() {
	if(dev) {
		printf("libusb_unref_device\n");
		libusb_unref_device(dev);
	}
}

device::operator libusb_device*() const {
	return dev;
}

device::operator bool() const {
	return dev;

}
/* ------------------------------------ */

device_descriptor device::descriptor() const {
	return device_descriptor(dev);
}

/* ------------------------------------ */

interface::interface(device_handle _handle,int _interface_number)
:handle(_handle),interface_number(_interface_number)
{
	printf("interface claim[%i]\n",interface_number);
	ret = handle.claim_interface(interface_number);
}

interface::~interface() {
	printf("interface release[%i]\n",interface_number);
	if(ret == 0) {
		handle.release_interface(interface_number);
	}
}

interface::operator bool() const {
	return ret == 0;
}

/* ------------------------------------ */

configuration::configuration(device_handle _handle, int _config_number)
:handle(_handle),config_number(_config_number)
{
	printf("configuration set[%i]\n",config_number);
	ret = handle.set_configuration(config_number);
}

configuration::~configuration() {
	printf("configuration set[%i]\n",-1);
	handle.set_configuration(-1);
}
	
configuration::operator bool() const {
	return ret == 0;
}

/* ------------------------------------ */

device_list::device_list(const context &ctx) {
	printf("libusb_get_device_list\n");
	count = libusb_get_device_list(ctx,&list);
	if(count <= 0) {
		printf("No usb devices found\n");
		count = 0;
	} else {
		printf("Found %i usb devices\n",count);
	}
}

device_list::~device_list() {
	if(count > 0) {
		printf("libusb_free_device_list\n");
		libusb_free_device_list(list,1);
	}
}

ssize_t device_list::get_count() const {
	return count;
}

device device_list::find(uint16_t vid, uint16_t pid) {

	auto checker = [&vid,&pid](device_descriptor desc) {
		return desc.idVendor == vid && desc.idProduct == pid;
	};

	std::vector<libusb_device*> devices(list,list + count);
	auto dev = std::find_if(devices.begin(),devices.end(),checker);
	
	return dev != devices.end() ? *dev : nullptr;
}

/* ------------------------------------ */

void transfer::init_native_transfer() {
	libusb_transfer *_tr = libusb_alloc_transfer(0);
	if(!_tr) {
		fprintf(stderr,"Unable to allocate transfer\n");
		return;	
	}
	
	auto deleter = [](libusb_transfer *t) {
		if(log_level) fprintf(stderr,"libusb_free_transfer[%p]\n",t);
		
		/*if(int ret = libusb_cancel_transfer(t)) {
			fprintf(stderr,"libusb_cancel_transfer[%s]\n",libusb_error_name(ret));
		}*/
		
		libusb_free_transfer(t);	
	};
	
	tr = boost::shared_ptr<libusb_transfer>(_tr,deleter);	
}

transfer::transfer(device_handle _dev,
                   uint8_t type,
				   uint8_t endpoint,			   
			       uint8_t flags,
			       unsigned int timeout)
:dev(_dev)
{
	init_native_transfer();
	
	tr->dev_handle = dev;
	tr->flags = flags;
	tr->endpoint = endpoint;
	tr->type = type;
	tr->timeout = timeout;
	tr->user_data = this;
	tr->callback = generic_callback;
	tr->num_iso_packets = 0;
	tr->buffer = 0;
	tr->length = 0;
}

transfer::~transfer() {
	if(log_level) fprintf(stderr,"~transfer\n");
}

transfer::operator bool() const {
	return tr;
}

void transfer::generic_callback(libusb_transfer *native_transfer) {
	//fprintf(stderr,"transfer::generic_callback[%p]\n",native_transfer);
	
	transfer *wrapper = (transfer*)native_transfer->user_data;	
	if(log_level) {
		fprintf(stderr,"transfer_completed[%s][%i/%i]\n",wrapper->status_str(),
	                                                     wrapper->actual_length(),
			    										 wrapper->length());
	}
	
	wrapper->transfer_completed(wrapper);
}

int transfer::submit() {
	return libusb_submit_transfer(tr.get());
}

int transfer::cancel() {
	return libusb_cancel_transfer(tr.get());
}

void transfer::fill_control(uint8_t bmRequestType,
	                        uint8_t bRequest,
				            uint16_t wValue,
				            uint16_t wIndex,
				            const void *data,
                            uint16_t wLength)
{
	const size_t buffer_len = sizeof(libusb_control_setup) + wLength;
	boost::shared_array<uint8_t> buffer(new uint8_t[buffer_len]);

	libusb_fill_control_setup(buffer.get(),bmRequestType,
	                                       bRequest,
										   wValue,
										   wIndex,
										   wLength);
	memcpy(buffer.get() + sizeof(libusb_control_setup),data,wLength);
	
	set_buffer(buffer,buffer_len);
}

boost::shared_array<uint8_t> transfer::buffer() {
	return data_buffer;
}

void transfer::set_buffer(boost::shared_array<uint8_t> buffer,size_t len) {
	data_buffer = buffer;
	tr->buffer = (unsigned char*)data_buffer.get();
	tr->length = len;
}

void transfer::set_buffer(void *buffer, size_t len) {
	set_buffer(boost::shared_array<uint8_t>((uint8_t*)buffer,[](uint8_t *) {
		//fprintf(stderr,"shared_array stub delete\n");
	}),len);
}

void transfer::allocate_buffer(size_t len) {
	boost::shared_array<uint8_t> buffer(new uint8_t[len],[len](uint8_t *arr) {
		fprintf(stderr,"deleting array[%p] len[%i]\n",arr,len);
		delete[] arr;
	});
	set_buffer(buffer,len);
}

uint8_t transfer::flags() const {
	return tr->flags;
}

void transfer::set_flags(uint8_t flags) {
	tr->flags = flags;
}

uint8_t transfer::endpoint() const {
	return tr->endpoint;
}

void transfer::set_endpoint(uint8_t endpoint) {
	tr->endpoint = endpoint;
}

uint8_t transfer::type() const {
	return tr->type;
}

void transfer::set_type(uint8_t type) {
	tr->type = type;
}

int transfer::status() const {
	return tr->status;
}

const char* transfer::status_str() const {
	switch(tr->status) {
		case LIBUSB_TRANSFER_COMPLETED: return "LIBUSB_TRANSFER_COMPLETED";
		case LIBUSB_TRANSFER_ERROR:     return "LIBUSB_TRANSFER_ERROR";
		case LIBUSB_TRANSFER_TIMED_OUT: return "LIBUSB_TRANSFER_TIMED_OUT";
		case LIBUSB_TRANSFER_CANCELLED: return "LIBUSB_TRANSFER_CANCELLED";
		case LIBUSB_TRANSFER_STALL:     return "LIBUSB_TRANSFER_STALL";
		case LIBUSB_TRANSFER_NO_DEVICE: return "LIBUSB_TRANSFER_NO_DEVICE";
		case LIBUSB_TRANSFER_OVERFLOW:  return "LIBUSB_TRANSFER_OVERFLOW";
		default: return "UNKNOWN";
	}
}

int transfer::length() const {
	return tr->length;
}

int transfer::actual_length() const {
	return tr->actual_length;
}

unsigned int transfer::timeout() const {
	return tr->timeout;
}

void transfer::set_timeout(unsigned int timeout) {
	tr->timeout = timeout;
}

/* ------------------------------------ */