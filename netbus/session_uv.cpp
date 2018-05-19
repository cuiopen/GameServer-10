#include "uv.h"
#include "session.h"
#include "session_uv.h"
#include "../utils/cache_alloc.h"
#include "ws_protocol.h"
#include "tcp_protocol.h"
#include "service_man.h"
#include "proto_man.h"


#include <iostream>
#include <string>

#define SESSION_CACHE_CAPACITY 6000
#define WQ_CACHE_CAPCITY 4096
#define WBUF_CACHE_CAPCITY 1024
#define CMD_CACHE_SIZE 1024

static CacheAllocer* session_allocer = NULL;
static CacheAllocer* wr_allocer = NULL;
CacheAllocer* wbuf_allocer = NULL;



extern "C"
{
	static void
	on_close(uv_handle_t* handle)
	{
		printf("close client\n");
		UVSession* s = (UVSession*)handle->data;
		UVSession::Destroy(s);
	}

	static void
	on_shutdown(uv_shutdown_t* req, int status)
	{
		uv_close((uv_handle_t*)req->handle, on_close);
	}


	static void
	write_after(uv_write_t* req, int status)
	{
		if (status == 0) 
		{
			printf("write success\n");
		}
		MyCacheAlloc::CacheFree(wr_allocer, req);
	}
}


void 
UVSession::Init()
{
	memset(&this->shutdown_t, 0, sizeof(uv_shutdown_t));
	memset(&this->tcp_handle, 0, sizeof(uv_tcp_t));
	memset(this->c_address, 0, sizeof(this->c_address));
	this->recved = 0;
	this->c_port = 0;
	isShutDown = false;
	this->is_ws_shake = 0;
	this->long_pkg = NULL;
	this->long_pkg_size = 0;
}


void 
UVSession::Exit()
{

}



UVSession*
UVSession::Create()
{
	UVSession* uv_s = (UVSession*)MyCacheAlloc::CacheAlloc(session_allocer, sizeof(UVSession));
	uv_s->UVSession::UVSession();

	uv_s->Init();
	return uv_s;
}


void 
UVSession::Destroy(UVSession* s)
{
	s->Exit();
	s->UVSession::~UVSession();
	MyCacheAlloc::CacheFree(session_allocer, s);
}


void 
UVSession::Close()
{
	if (this->isShutDown)
	{
		return;
	}
	ServiceMan::OnSessionDisconnect(this);
	this->isShutDown = true;
	uv_shutdown_t* reg = &this->shutdown_t;
	uv_shutdown(reg, (uv_stream_t*)&this->tcp_handle, on_shutdown);
}


void 
UVSession::SendData(unsigned char* body, int len)
{
	uv_write_t* w_req = (uv_write_t*)MyCacheAlloc::CacheAlloc(wr_allocer, sizeof(uv_write_t));
	uv_buf_t w_buf;

	if (this->socket_type == WS_SOCKET) 
	{
		if (this->is_ws_shake)
		{
			int ws_pkg_len;
			unsigned char* ws_pkg = WSProtocol::PackageWSSendData(body, len, &ws_pkg_len);
			w_buf = uv_buf_init((char*)ws_pkg, ws_pkg_len);
			uv_write(w_req, (uv_stream_t*)&this->tcp_handle, &w_buf, 1, write_after);
			WSProtocol::FreeWSSendPkg(ws_pkg);
		}
		else
		{
			w_buf = uv_buf_init((char*)body, len);
			uv_write(w_req, (uv_stream_t*)&this->tcp_handle, &w_buf, 1, write_after);
		}
		
	}
	else 
	{
		int tcp_pkg_len;
		unsigned char* ws_pkg = TcpProtocol::Package(body, len, &tcp_pkg_len);
		w_buf = uv_buf_init((char*)ws_pkg, tcp_pkg_len);
		uv_write(w_req, (uv_stream_t*)&this->tcp_handle, &w_buf, 1, write_after);
		TcpProtocol::ReleasePackage(ws_pkg);
	}
}


const char* 
UVSession::GetAddress(int* client_port)
{
	*client_port = this->c_port;
	return this->c_address;
}

void 
UVSession::InitSessionAllocer() {
	if (session_allocer == NULL) 
	{
		session_allocer = MyCacheAlloc::CreateCacheAllocer(SESSION_CACHE_CAPACITY, sizeof(UVSession));
	}

	if (wr_allocer == NULL) 
	{
		wr_allocer = MyCacheAlloc::CreateCacheAllocer(WQ_CACHE_CAPCITY, sizeof(uv_write_t));
	}
	if (wbuf_allocer == NULL)
	{
		wbuf_allocer = MyCacheAlloc::CreateCacheAllocer(WBUF_CACHE_CAPCITY, CMD_CACHE_SIZE);
	}
}

void 
UVSession::SendMsg(CmdMsg* msg)
{
	unsigned char* encode_pkg = NULL;
	int len1;
	encode_pkg=ProtoMan::EncodeMsgToRaw(msg, &len1);
	if (encode_pkg)
	{
		this->SendData(encode_pkg, len1);
		ProtoMan::MsgRawFree(encode_pkg);
	}
}