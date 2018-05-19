#ifndef __WS_PROTOCOL_H__
#define __WS_PROTOCOL_H__

class Session;
class WSProtocol {
public:
	static bool WSShakeHand(Session* s, char* body, int len);
	static bool ReadWSHeader(unsigned char* pkg_data, int pkg_len, int* pkg_size, int* out_header_size);
	static void ParserWSRecvData(unsigned char* raw_data, unsigned char* mask, int raw_len);
	static unsigned char* PackageWSSendData(const unsigned char* raw_data, int len, int* ws_data_len);
	static void FreeWSSendPkg(unsigned char* ws_pkg);
};


#endif