#ifndef __SERVICE_H__
#define __SERVICE_H__

struct CmdMsg;
class Session;

class Service 
{
public:
	virtual bool OnSessionRecvCmd(Session* s, struct CmdMsg* msg);
	virtual void OnSessionDisconnect(Session* s);
};





#endif // !__SERVICE_H__
