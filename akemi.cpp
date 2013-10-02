#include <stdio.h>
#include <algorithm>
#include <vector>
#include <usb.h>
#include <cp210x.h>

int main(int argc, char **argv)
{
	usb::context context(3);
	
	cp210x cp(context,false);
	if(!cp) {
		printf("No cp210x device found\n");
		return 1;
	}
	
	cp.set_baud(38400);
    cp.set_ctl(cp210x::data8 | cp210x::parity_none | cp210x::stop1);
	
	printf("Baud rate from device: %i\n",cp.get_baud());
	printf("Ctl from device: %hi\n",cp.get_ctl());
	
	uint8_t get_sn_request[] = {0xff,0,0x10,0,0x10,0xcc};		
	
	auto send_handler = [](int status, size_t len) {
		fprintf(stderr,"data_sent[%i][%i]\n",status,len);	
	};
	
	cp.data_received.connect([&](int status, void *data, size_t len) {
		auto bytes_str = usb::format_bytes(data,len);
		fprintf(stderr,"data_received[%i][%s]\n",status,bytes_str.get());
		if(!status) {
			cp.send_async(get_sn_request,sizeof(get_sn_request),send_handler);
			cp.recv_async();
		}
	});
	
	cp.recv_async();
	cp.send_async(get_sn_request,sizeof(get_sn_request),send_handler);
	
	
	sleep(60);
	
	
	return 0;
	
	/*
	if(argc > 1) {
	
		if(cp.set_product_string(argv[1]) < 0) {
			printf("set_product_string failed\n");
			return 2;
		}
	
	} else {
	
		char product_string[100] = {0};
		if(cp.get_product_string(product_string,sizeof(product_string)) < 0) {
			printf("set_product_string failed\n");
			return 3;
		}
		
		printf("Product string: %s\n",product_string);	
	}*/
		
	return 0;
}
