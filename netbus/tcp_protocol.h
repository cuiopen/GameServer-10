#ifndef __TCP_PROTOCOL_H__
#define __TCP_PROTOCOL_H__
class TcpProtocol {
public:
	static bool ReadHeader(unsigned char* data, int data_len, int* pkg_size, int* out_header_size);
	static unsigned char* Package(const unsigned char* raw_data, int len, int* pkg_len);
	static void ReleasePackage(unsigned char* tp_pkg);
};



#endif // !__TCP_PROTOCOL_H__
