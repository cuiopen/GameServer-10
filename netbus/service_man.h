#ifndef __SERVICE_MAN_H__
#define __SERVICE_MAN_H__

struct CmdMsg;
class Session;
class Service;

class ServiceMan
{
public:
	static void Init();
	static bool RegisterService(int stype, Service* s);
	static bool OnRecvCmdMsg(Session* s, struct CmdMsg* msg);
	static void OnSessionDisconnect(Session* s);
};






#endif // !__SERVICE_H__
