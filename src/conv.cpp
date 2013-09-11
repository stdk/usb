#include <conv.h>

#include <stdio.h>
#include <errno.h>

conv::conv(const char *tocode, const char *fromcode) {
	cd = iconv_open(tocode,fromcode);		
	if((iconv_t)-1 == cd) {
		perror("iconv_open");
	}
}

conv::~conv() {
	if(cd != (iconv_t)-1) {
		iconv_close(cd);
	}
}

size_t conv::convert(char *inbuf, size_t *inbytesleft,char *outbuf, size_t *outbytesleft) {
	size_t ret = iconv(cd,&inbuf,inbytesleft,&outbuf,outbytesleft);
	if((size_t)-1 == ret) {
		perror("iconv");
	}
	return ret;
}

boost::shared_array<char> conv::convert(const char* inbuf,
                                        size_t inbytesleft,
										size_t buflen,
										size_t *outlen)
{
	boost::shared_array<char> buffer(new char[buflen]);
	
	size_t outbytesleft = buflen;
	size_t ret = convert(const_cast<char*>(inbuf),&inbytesleft,buffer.get(),&outbytesleft);
	if(outlen) *outlen = buflen - outbytesleft;
	
	return buffer;
}

conv::operator bool() const {
	return cd != (iconv_t)-1;
}