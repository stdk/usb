#ifndef CONVERTER_H
#define CONVERTER_H

#include <iconv.h>
#include <boost/shared_array.hpp>

class conv
{
	iconv_t cd;
public:
	conv(const char *tocode, const char *fromcode);
	~conv();		
	
	boost::shared_array<char> convert(const char* inbuf,
	                                  size_t inbytesleft,
									  size_t buflen = 128,
									  size_t *outlen = nullptr);
	
	size_t convert(char *inbuf, size_t *inbytesleft,char *outbuf, size_t *outbytesleft);
	
	operator bool() const;
};

#endif //CONVERTER_H