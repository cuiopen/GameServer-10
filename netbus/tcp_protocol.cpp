#include "tcp_protocol.h"
#include "../utils/cache_alloc.h"
extern CacheAllocer* wbuf_allocer;

#include <iostream>
#include <stdlib.h>



bool TcpProtocol::ReadHeader(unsigned char* data, int data_len, int* pkg_size, int* out_header_size)
{
	if (data_len < 2)
	{
		return false;
	}
	*pkg_size = (data[0] | data[1] << 8);
	*out_header_size = 2;

	return true;
}

unsigned char* TcpProtocol::Package(const unsigned char* raw_data, int len, int* pkg_len)
{
	int head_size = 2;
	*pkg_len = (head_size + len);
	// cache malloc
	unsigned char* data_buf = (unsigned char*)MyCacheAlloc::CacheAlloc(wbuf_allocer,(*pkg_len));
	data_buf[0] = (unsigned char)((*pkg_len) & 0x000000ff);
	data_buf[1] = (unsigned char)(((*pkg_len) & 0x0000ff00) >> 8);;
	memcpy(data_buf + head_size, raw_data, len);
	return data_buf;
}

void TcpProtocol::ReleasePackage(unsigned char* tp_pkg)
{
	MyCacheAlloc::CacheFree(wbuf_allocer, tp_pkg);
}

