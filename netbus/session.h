#ifndef __SESSION_H__
#define __SESSION_H__

struct CmdMsg;

class Session
{

public:
	virtual void Close() = 0;
	virtual void SendData(unsigned char* body, int len) = 0;
	virtual const char* GetAddress(int* client_port) = 0;
	virtual void SendMsg(CmdMsg* msg) = 0;
};


#endif // !__SESSION_H__
