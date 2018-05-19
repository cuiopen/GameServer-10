#ifndef __SESSION_UV_H__
#define __SESSION_UV_H__
#define MAX_RECV_LEN 4096

struct CmdMsg;

enum Session_Type
{
	TCP_SOCKET=1,
	WS_SOCKET=2
};


class UVSession : Session
{
public:
	uv_tcp_t tcp_handle;
	uv_shutdown_t shutdown_t;
	char c_address[32];
	int c_port;
	char recv_buf[MAX_RECV_LEN];
	int recved;
	int socket_type;
	bool isShutDown;
	char* long_pkg;
	int long_pkg_size;
	int is_ws_shake;
private:
	void Init();
	void Exit();
public:
	static void InitSessionAllocer();
	static UVSession* Create();
	static void Destroy(UVSession* s);

public:
	virtual void Close();
	virtual void SendData(unsigned char* body, int len);
	virtual const char* GetAddress(int* client_port);
	virtual void SendMsg(CmdMsg* msg);
};

#endif // !__SESSION_UV_H
