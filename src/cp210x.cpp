#include <cp210x.h>
#include <stdio.h>
#include <string.h>
#include <conv.h>

#include <boost/shared_array.hpp>
#include <boost/bind.hpp>

#define VENDOR_ID 0x10c4
#define PRODUCT_ID 0xea60

#define USB_CTRL_SET_TIMEOUT    5000
#define USB_CTRL_GET_TIMEOUT    5000

#define REQTYPE_HOST_TO_INTERFACE (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT | 0x01)
#define REQTYPE_INTERFACE_TO_HOST (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN  | 0x01)

#define REQTYPE_HOST_TO_DEVICE (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT | 0x00)
#define REQTYPE_DEVICE_TO_HOST (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN  | 0x00) 

#define UART_ENDPOINT_IN        (LIBUSB_ENDPOINT_IN | 0x01)
#define UART_ENDPOINT_OUT       (LIBUSB_ENDPOINT_OUT | 0x01)

#define CP2101_UART             0x00
#define CP2101_CONFIG           0xff

#define CP2101_UART_ENABLE      0x0001
#define CP2101_UART_DISABLE     0x0000

#define REG_VENDOR_ID           0x3701
#define REG_PRODUCT_ID          0x3702
#define REG_PRODUCT_STRING      0x3703
#define REG_SERIAL_NUMBER       0x3704

#define SIZE_PRODUCT_STRING     0x007d
#define SIZE_SERIAL_NUMBER      0x003f

#define CP210X_IFC_ENABLE       0x00
#define CP210X_SET_BAUDDIV      0x01
#define CP210X_GET_BAUDDIV      0x02
#define CP210X_SET_LINE_CTL     0x03
#define CP210X_GET_LINE_CTL     0x04
#define CP210X_SET_FLOW         0x13
#define CP210X_GET_FLOW         0x14
#define CP210X_GET_BAUDRATE     0x1d
#define CP210X_SET_BAUDRATE     0x1e
#define CP210X_PURGE            0x12

#define UART_ENABLE             0x0001
#define UART_DISABLE            0x0000

#define PURGE_TRANSMIT_QUEUE    (1 << 0)
#define PURGE_RECEIVE_QUEUE     (1 << 1)

usb::device cp210x::find_device(const usb::context &context, uint16_t vid, uint16_t pid)
{
	usb::device_list devices(context);
	
	return devices.find(vid,pid);
}

cp210x::cp210x(const usb::context &ctx, bool _auto_recv)
    : context(ctx),
	  device(find_device(context,VENDOR_ID,PRODUCT_ID)),
      handle(device,true),
	  conf(handle,1),
	  interface_number(0),
	  iface(handle,interface_number),
	  recv_transfer(handle,LIBUSB_TRANSFER_TYPE_BULK,UART_ENDPOINT_IN,0,1000),
	  completed(0),
	  auto_recv(_auto_recv) {
	if(!handle) {
		fprintf(stderr,"Cannot open cp210x device\n");
		return;
	}
	//printf("cp210x device descriptor:\n");
	//device.descriptor().print("-> ");
	
	if(set_interface_config_single(CP210X_IFC_ENABLE,UART_ENABLE)) {
		printf("Cannot enable UART\n");
	}
	
	if(set_interface_config_single(CP210X_PURGE,PURGE_TRANSMIT_QUEUE | PURGE_RECEIVE_QUEUE)) {
		printf("Cannot purge device io queues\n");
	}
	
	auto recv_callback = [this](usb::transfer *tr) {
		//fprintf(stderr,"cp210x::recv_callback\n");
		this->data_received(tr->status(),tr->buffer().get(),tr->actual_length());
		
		if(this->auto_recv) {
			tr->submit();
		}
	};
	
	recv_transfer.allocate_buffer(64);
	recv_transfer.transfer_completed.connect(recv_callback);
	
	io_thread = boost::thread(boost::bind(&cp210x::io_thread_func,this));
}

cp210x::~cp210x() {
	completed = 1;
	//io_thread.interrupt();
	io_thread.join();
}

void cp210x::io_thread_func() {
	if(auto_recv) {
		recv_transfer.submit();
	}
	
	while(!completed) {
		//fprintf(stderr,"io_thread_func\n");
		//boost::this_thread::interruption_point();
		
		struct timeval tv = { 3, 0 };
		//context.handle_events_timeout(&tv);
		libusb_handle_events_timeout_completed(context,&tv,&completed);
	}
	
	fprintf(stderr,"io_thread_func stop\n");
	
	data_received.disconnect_all_slots();
		
	recv_transfer.cancel();
		
	for(size_t i = 0; i < 2; i++) {
		struct timeval tv = { 0, 0 };
		context.handle_events_timeout(&tv);
	}		

	fprintf(stderr,"io_thread_func completed\n");
}

uint32_t cp210x::get_baud() {
	uint32_t baud = 0;
	if(get_interface_config(CP210X_GET_BAUDRATE,&baud,sizeof(baud)) != sizeof(baud)) {
		printf("Cannot retrieve baud\n");	
	} else {
		printf("Current baud = %i\n",baud);
	}
	return baud;
}

int cp210x::set_baud(uint32_t baud) {
	return set_interface_config(CP210X_SET_BAUDRATE,&baud,sizeof(baud)) == sizeof(baud);	
}

uint16_t cp210x::get_ctl() {
	uint16_t ctl = 0;
	if(get_interface_config(CP210X_GET_LINE_CTL,&ctl,sizeof(ctl)) != sizeof(ctl)) {
		printf("Cannot retrieve ctl\n");	
	} else {
		switch(ctl & data_mask) {
		case data5:
			printf("data bits = 5\n");
			break;
		case data6:
			printf("data bits = 6\n");
			break;
		case data7:
			printf("data bits = 7\n");
			break;
		case data8:
			printf("data bits = 8\n");
			break;
		case data9:
			printf("data bits = 9\n");
			break;
		default:
			printf("data bits = unknown\n");
			break;
		}
		
		switch(ctl & parity_mask) {
		case parity_none:
			printf("parity = NONE\n");
			break;
		case parity_odd:
			printf("parity = ODD\n");
			break;
		case parity_even:
			printf("parity = EVEN\n");
			break;
		case parity_mark:
			printf("parity = MARK\n");
			break;
		case parity_space:
			printf("parity = SPACE\n");
			break;
		default:
			printf("parity = unknown\n");
			break;
		}
		
		switch(ctl & stop_mask) {
		case stop1:
			printf("stop bits = 1\n");
			break;
		case stop1_5:
			printf("stop bits = 1.5\n");
			break;
		case stop2:
			printf("stop bits = 2\n");
			break;
		default:
			printf("stop bits = unknown\n");
			break;
		}
	}
	return ctl;
}

int cp210x::set_ctl(uint16_t ctl) {
	return set_interface_config(CP210X_SET_LINE_CTL,&ctl,sizeof(ctl)) == sizeof(ctl);	
}

/*int cp210x::send(void *buffer, size_t len, uint32_t timeout) {
	int transferred = 0;
	
	int ret = handle.bulk_transfer(UART_ENDPOINT_OUT,buffer,len,&transferred,timeout);
	
	return ret ? -1 : transferred;
}*/

/*int cp210x::recv(void *buffer, size_t len, uint32_t timeout) {
	int transferred = 0;
	
	int ret = handle.bulk_transfer(UART_ENDPOINT_IN,buffer,len,&transferred,timeout);
	
	return ret ? -1 : transferred;
}*/

int cp210x::recv_async() {
	return recv_transfer.submit();
}

/*
class cp210x_send_transfer_handler
{
	boost::shared_ptr<usb::transfer> t;
	boost::function<void (int status, size_t bytes_transferred)> callback;
public:
	cp210x_send_transfer_handler(boost::shared_ptr<usb::transfer> _t,
	 boost::function<void (int status, size_t bytes_transferred)> _callback)
	:t(_t),callback(_callback) {
	
	}

	void operator()(boost::signals2::connection c, usb::transfer *tr) {
		fprintf(stderr,"A callback use_count = %i\n",t.use_count());
		fprintf(stderr,"cp210x::send_callback[%p]\n",tr);
		callback(tr->status(),tr->actual_length());
		c.disconnect();
		//tr->transfer_completed.disconnect_all_slots();
		t.reset();
		fprintf(stderr,"B callback use_count = %i\n",t.use_count());
	}
};*/

int cp210x::send_async(void *buffer, size_t len,
                       boost::function<void (int status, size_t bytes_transferred)> callback, 
					   uint32_t timeout) {
	boost::shared_ptr<usb::transfer> t(new usb::transfer(handle,LIBUSB_TRANSFER_TYPE_BULK,UART_ENDPOINT_OUT,0,timeout));
	t->set_buffer(buffer,len);
	
	auto handler = [t,callback](boost::signals2::connection c,usb::transfer *tr) mutable {
		fprintf(stderr,"A callback use_count = %i\n",t.use_count());
		fprintf(stderr,"cp210x::send_callback[%p]\n",tr);
		callback(tr->status(),tr->actual_length());
		t->transfer_completed.disconnect_all_slots();
		c.disconnect();
		//tr->transfer_completed.disconnect_all_slots();
		t.reset();
		fprintf(stderr,"B callback use_count = %i\n",t.use_count());
		
	};
	
	//t->transfer_completed.connect_extended(handler);
	t->transfer_completed.connect_extended(handler);
	
	fprintf(stderr,"use_count = %i\n",t.use_count());
	
	return t->submit();
}

int cp210x::set_interface_config(uint8_t request, const void *data, size_t len) {
    uint16_t index = 0;
    if(len == sizeof(index)) {
		index = ((uint16_t*)data)[0];
		data = 0;
		len = 0;
	}
	
	return handle.control_transfer(REQTYPE_HOST_TO_INTERFACE,
	                               request,
								   index,
								   interface_number,
								   data,
								   len,
								   USB_CTRL_SET_TIMEOUT);
}

int cp210x::get_interface_config(uint8_t request, void *data, size_t len) {
	return handle.control_transfer(REQTYPE_INTERFACE_TO_HOST,
	                               request,
	                               0,
								   interface_number,
								   data,
								   len,
								   USB_CTRL_GET_TIMEOUT);
}

int cp210x::set_interface_config_single(uint8_t request, unsigned int data) {
	return set_interface_config(request, &data, 2);
}

int cp210x::set_device_config(uint16_t value, uint16_t index,const void *data, size_t len) {
	return handle.control_transfer(REQTYPE_HOST_TO_DEVICE,
	                               CP2101_CONFIG,value,index,data,len,USB_CTRL_SET_TIMEOUT);
}

int cp210x::get_device_config(uint16_t value, uint16_t index,void *data, size_t len) {
	return handle.control_transfer(REQTYPE_DEVICE_TO_HOST,
	                               CP2101_CONFIG,value,index,data,len,USB_CTRL_GET_TIMEOUT);
}

int cp210x::set_config_string(uint16_t value, size_t max_length, char* data) {
	conv cd("UTF-16LE","UTF8");
	if(!cd) return -1;
	
	boost::shared_array<char> buffer(new char[max_length + 2]);
	size_t outbytesleft = max_length;
	size_t inbytesleft = strlen(data);
	if((size_t)-1 == cd.convert(data,&inbytesleft,buffer.get() + 2,&outbytesleft)) return -1;
	
	size_t length = max_length - outbytesleft;
	buffer[0] = length + 2;
	buffer[1] = 3;
	
	return set_device_config(value,interface_number,buffer.get(),length + 2);
}

int cp210x::set_product_string(char* s) {
	return set_config_string(REG_PRODUCT_STRING,SIZE_PRODUCT_STRING,s);
}

int cp210x::get_product_string(char *s, size_t len) {
	usb::config_descriptor desc(device);
	
	//printf("bNumInterfaces = %i\n",desc->bNumInterfaces);
	
	auto interface = desc->interface->altsetting;
	
	//auto endpoint0 = interface->endpoint[0];
	//printf("0: bEndpointAddress = %i\n",endpoint0.bEndpointAddress);
	
	//auto endpoint1 = interface->endpoint[1];
	//printf("1: bEndpointAddress = %i\n",endpoint1.bEndpointAddress);

	return libusb_get_string_descriptor_ascii(handle,interface->iInterface,(unsigned char*)s,len);
}

cp210x::operator bool() const {
	return handle;
}