#include <stdio.h>
#include <usb.h>

usb::device find_device(usb::context context, uint16_t vid, uint16_t pid)
{
	usb::device_list devices(context);
	
	return devices.find(vid,pid);
}

int main(int argc, char **argv)
{
	if(argc != 3) {
		printf("Usage: info <VENDOR_ID> <PRODUCT_ID>\n");
		return 1;
	}

	uint16_t vid = strtol(argv[1],0,16);
	uint16_t pid = strtol(argv[2],0,16);

	usb::context context(3);
	
	usb::device cp = find_device(context,vid,pid);
	if(!cp) {
		printf("No device[VID=0x%hX][PID=0x%hX] found\n",vid,pid);
		return 2;
	}
	
	auto desc = usb::device_descriptor(cp);
	if(!desc) {
		printf("Cannot read device[VID=0x%hX][PID=0x%hX] hardware descriptor\n",vid,pid);
		return 3;
	}
	
	desc.print();
	
	//usb::device_handle handle(cp);
	//handle.set_configuration(0);
	
	printf("Config descriptor:\n");
	auto conf = usb::config_descriptor(cp);
	if(!conf) {
		printf("Cannot read device config descriptor\n");
		return 4;
	}
	
	printf("bLength            = 0x%hhX\n",conf->bLength);
	printf("bDescriptorType    = 0x%hhX\n",conf->bDescriptorType);
	printf("wTotalLength       = 0x%hX\n",conf->wTotalLength);
	printf("bNumInterfaces     = 0x%hhX\n",conf->bNumInterfaces);
	printf("bConfigurationValue= 0x%hhX\n",conf->bConfigurationValue);
	printf("iConfiguration     = 0x%hhX\n",conf->iConfiguration);
	printf("bmAttributes       = 0x%hhX\n",conf->bmAttributes);
	printf("MaxPower           = 0x%hhX\n",conf->MaxPower);
	
	auto iface = conf->interface->altsetting;
	printf("Interface 0:\n");
	printf("bLength            = 0x%hhX\n",iface->bLength);
	printf("bDescriptorType    = 0x%hhX\n",iface->bDescriptorType);
	printf("bInterfaceNumber   = 0x%hhX\n",iface->bInterfaceNumber);
	printf("bAlternateSetting  = 0x%hhX\n",iface->bAlternateSetting);
	printf("bNumEndpoints      = 0x%hhX\n",iface->bNumEndpoints);
	printf("bInterfaceClass    = 0x%hhX\n",iface->bInterfaceClass);
	printf("bInterfaceSubClass = 0x%hhX\n",iface->bInterfaceSubClass);
	printf("iInterface         = 0x%hhX\n",iface->iInterface);
	
	for(size_t i = 0; i < iface->bNumEndpoints;i++) {
		auto endpoint = iface->endpoint[i];
		printf("Endpoint[%i]\n",i);
		printf("bLength            = 0x%hhX\n",endpoint.bLength);
		printf("bDescriptorType    = 0x%hhX\n",endpoint.bDescriptorType);
		printf("bEndpointAddress   = 0x%hhX\n",endpoint.bEndpointAddress);
		printf("bmAttributes       = 0x%hhX\n",endpoint.bmAttributes);
		printf("wMaxPacketSize     = 0x%hX\n",endpoint.wMaxPacketSize);
		printf("bInterval          = 0x%hhX\n",endpoint.bInterval);
		printf("bRefresh           = 0x%hhX\n",endpoint.bRefresh);
		printf("bSynchAddress      = 0x%hhX\n",endpoint.bSynchAddress);
	}
	
	return 0;
}
