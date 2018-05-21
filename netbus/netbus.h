#ifndef __NETBUS_H__
#define __NETBUS_H__
class Netbus
{
public:
	static Netbus* instance();

public:
	void Init();
	void StartTcpServer(int port);
	void StartUdpServer(int port);
	void StartWSServer(int port);//websocketÆô¶¯½Ó¿Ú
	void Run();
private:

};
#endif // __NETBUS_H__


