#ifndef __UDP_Session_H__
#define __UDP_Session_H__



class UdpSession : Session 
{
public:
	uv_udp_t* udp_handler;
	char c_address[32];
	int c_port;
	const struct sockaddr* addr;

public:
	virtual void Close();
	virtual void SendData(unsigned char* body, int len);
	virtual const char* GetAddress(int* client_port);
	virtual void SendMsg(struct CmdMsg* msg);
};


#endif // !__UDP_Session_H__


