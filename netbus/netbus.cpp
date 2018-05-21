#include "uv.h"
#include "netbus.h"
#include "session.h"
#include "session_uv.h"
#include "ws_protocol.h"
#include "tcp_protocol.h"
#include "proto_man.h"
#include "service_man.h"
#include "udp_session.h"

#include <iostream>
#include <string>

static Netbus g_netbus;

Netbus* 
Netbus::instance()
{
	return &g_netbus;
}

extern"C"
{
	struct udp_recv_buf
	{
		char* recv_buf;
		size_t max_recv_len;

	};


	static void
	udp_uv_alloc_buf(uv_handle_t* handle,
					 size_t suggested_size,
					 uv_buf_t* buf)
	{
		udp_recv_buf* udp_buf = (udp_recv_buf*)handle->data;
		if (udp_buf->max_recv_len<suggested_size)
		{
			if (udp_buf->recv_buf)
			{
				free(udp_buf->recv_buf);
				udp_buf->recv_buf = NULL;
			}
			udp_buf->recv_buf = (char*)malloc(suggested_size);
			udp_buf->max_recv_len = suggested_size;
		}
		buf->base = udp_buf->recv_buf;
		buf->len = suggested_size;
	}



	static void 
	on_recv_client_cmd (Session* s, unsigned char* body, int len)
	{
		printf("client command !!!!\n");

		//test
		CmdMsg* msg = NULL;
		
		if (ProtoMan::DecodeCmdMsg(body, len, &msg))
		{
			//s->SendMsg(msg);
			if (!ServiceMan::OnRecvCmdMsg((Session*)s, msg))
			{
				//s->Close();
				ProtoMan::CmdMsgFree(msg);
			}
			
		}
	}
	static void
		after_recv(uv_udp_t* handle,
			ssize_t nread,
			const uv_buf_t* buf,
			const struct sockaddr* addr,
			unsigned flags)
	{
		UdpSession udp_s;
		udp_s.udp_handler = handle;
		udp_s.addr = addr;
		uv_ip4_name((struct sockaddr_in*)addr, udp_s.c_address, 32);
		udp_s.c_port = ntohs(((struct sockaddr_in*)addr)->sin_port);
		on_recv_client_cmd((Session*)&udp_s, (unsigned char*)buf->base, nread);
	}

	static void 
	on_recv_tcp_data(UVSession* s)
	{
		unsigned char* pkg_data = (unsigned char*)((s->long_pkg != NULL) ? s->long_pkg : s->recv_buf);

		while (s->recved > 0)
		{
			int pkg_size = 0;
			int head_size = 0;

			// pkg_size - head_size = body_size;
			if (!TcpProtocol::ReadHeader(pkg_data, s->recved, &pkg_size, &head_size))
			{
				break;
			}

			if (s->recved < pkg_size)
			{
				break;
			}

			unsigned char* raw_data = pkg_data + head_size;
			// recv client command;
			on_recv_client_cmd((Session*)s, raw_data, pkg_size - head_size);
			// end 

			if (s->recved > pkg_size)
			{
				memmove(pkg_data, pkg_data + pkg_size, s->recved - pkg_size);
			}
			s->recved -= pkg_size;

			if (s->recved == 0 && s->long_pkg != NULL)
			{
				free(s->long_pkg);
				s->long_pkg = NULL;
				s->long_pkg_size = 0;
			}
		}
	}


	static void on_recv_ws_data (UVSession* s)
	{
		unsigned char* pkg_data = (unsigned char*)((s->long_pkg != NULL) ? s->long_pkg : s->recv_buf);

		while (s->recved > 0)
		{
			int pkg_size = 0;
			int head_size = 0;

			if (pkg_data[0] == 0x88) 
			{ // close协议
				s->Close();
				break;
			}

			// pkg_size - head_size = body_size;
			if (!WSProtocol::ReadWSHeader(pkg_data, s->recved, &pkg_size, &head_size))
			{
				break;
			}

			if (s->recved < pkg_size)
			{
				break;
			}

			unsigned char* raw_data = pkg_data + head_size;
			unsigned char* mask = raw_data - 4;
			WSProtocol::ParserWSRecvData(raw_data, mask, pkg_size - head_size);
			// recv client command;
			on_recv_client_cmd((Session*)s, raw_data, pkg_size - head_size);
			// end 

			if (s->recved > pkg_size) 
			{
				memmove(pkg_data, pkg_data + pkg_size, s->recved - pkg_size);
			}
			s->recved -= pkg_size;

			if (s->recved == 0 && s->long_pkg != NULL)
			{
				free(s->long_pkg);
				s->long_pkg = NULL;
				s->long_pkg_size = 0;
			}
		}
	}

	static void 
	read_after(uv_stream_t* stream,
		       ssize_t nread,
		       const uv_buf_t* buf)
	{
		UVSession* s = (UVSession*)stream->data;
		if (nread < 0) 
		{
			s->Close();
			return;
		}
		// end
		s->recved += nread;
		if (s->socket_type == WS_SOCKET) 
		{// websocket
			if (s->is_ws_shake == 0) 
			{ // shake handle
				if (WSProtocol::WSShakeHand((Session*)s, s->recv_buf, s->recved)) {
					s->is_ws_shake = 1;
					s->recved = 0;
				}
			}
			else 
			{ // websocket recv/send data
				on_recv_ws_data(s);
				(s);
			}
		}
		else
		{
			on_recv_tcp_data(s);
		}
	}



	static void
	alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf)//分配空间时的回掉函数
	{
		UVSession* s = (UVSession*)handle->data;
		if (s->recved < MAX_RECV_LEN) 
		{
			*buf = uv_buf_init(s->recv_buf + s->recved, MAX_RECV_LEN - s->recved);	
		} 
		else //如果包的大小超过最大接收长度
		{
			if (s->long_pkg == NULL) 
			{ // alloc mem
				if (s->socket_type == WS_SOCKET && s->is_ws_shake) 
				{ // ws > RECV_LEN's package
					int pkg_size;
					int head_size;
					WSProtocol::ReadWSHeader((unsigned char*)s->recv_buf, s->recved, &pkg_size, &head_size);
					s->long_pkg_size = pkg_size;
					s->long_pkg = (char*)malloc(pkg_size);//直接分配出分配长包空间
					memcpy(s->long_pkg, s->recv_buf, s->recved);//将第一次收到的4096个字符拷贝到long_pkg
				}
				else 
				{ 
					// tcp > RECV_LEN's package
				}
			}
			*buf = uv_buf_init(s->long_pkg + s->recved, s->long_pkg_size - s->recved);//为其分配剩余需要接收包的空间,直接将buf指针指向long_pkg4096个字节之后的位置。
		}
	}

	static void
	my_uv_con(uv_stream_t* server, int status)
	{
		UVSession* s = UVSession::Create();
		s->socket_type = (int)(server->data);
		uv_tcp_t* client = &s->tcp_handle; //为客户端分配一个socket
		uv_tcp_init(uv_default_loop(), client);//把客户端句柄加入到loop中
		client->data = (void*)s;
		int r = uv_accept(server, (uv_stream_t*)client); //监听客户端接入
		struct sockaddr_in addr;
		int len = sizeof(addr);
		uv_tcp_getpeername(client, (sockaddr*)&addr, &len);
		uv_ip4_name(&addr, (char*)s->c_address, 64);
		s->c_port = ntohs(addr.sin_port);
		std::cout << "hava a client joined "<< s->c_address <<":"<<s->c_port << std::endl;
		uv_read_start((uv_stream_t*)client, alloc_cb, read_after);
	}
}

void 
Netbus::StartUdpServer(int port)
{
	uv_udp_t* server = (uv_udp_t*)malloc(sizeof(uv_udp_t));
	memset(server, 0, sizeof(uv_udp_t));

	uv_udp_init(uv_default_loop(), server);
	udp_recv_buf* buf = (udp_recv_buf*)malloc(sizeof(udp_recv_buf));
	memset(buf, 0, sizeof(udp_recv_buf));
	server->data = (udp_recv_buf*)buf;

	sockaddr_in addr;
	uv_ip4_addr("127.0.0.1", port, &addr);
	int ret = uv_udp_bind(server, (const struct sockaddr*)&addr, 0);

	uv_udp_recv_start(server, udp_uv_alloc_buf, after_recv);
}

void 
Netbus::StartTcpServer(int port)
{
	uv_tcp_t *m_listen = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	memset(m_listen, 0, sizeof(uv_tcp_t));
	uv_loop_t *t = uv_default_loop();
	uv_tcp_init(t, m_listen);
	sockaddr_in addr;
	uv_ip4_addr("127.0.0.1", port, &addr);
	int ret = uv_tcp_bind(m_listen, (const struct sockaddr*)&addr, 0);
	if (ret!=0)
	{
		std::cout << "bind failed" << std::endl;
		free(m_listen);
		return;
	}
	m_listen->data = (void*)TCP_SOCKET;
	uv_listen((uv_stream_t*)m_listen, SOMAXCONN, my_uv_con);
}


void Netbus::StartWSServer(int port) {
	uv_tcp_t* listen = (uv_tcp_t*)malloc(sizeof(uv_tcp_t));
	memset(listen, 0, sizeof(uv_tcp_t));
	uv_tcp_init(uv_default_loop(), listen);
	struct sockaddr_in addr;
	uv_ip4_addr("0.0.0.0", port, &addr);
	int ret = uv_tcp_bind(listen, (const struct sockaddr*) &addr, 0);
	if (ret != 0) {
		printf("bind error\n");
		free(listen);
		return;
	}
	listen->data = (void*)WS_SOCKET;
	uv_listen((uv_stream_t*)listen, SOMAXCONN, my_uv_con);
}



void 
Netbus::Run()
{
	uv_run(uv_default_loop(), UV_RUN_DEFAULT);
}

void
Netbus::Init()
{
	ServiceMan::Init();
	UVSession::InitSessionAllocer();
}

