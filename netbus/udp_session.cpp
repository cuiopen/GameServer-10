#include "uv.h"
#include "session.h"
#include "udp_session.h"
#include "../utils/cache_alloc.h"
#include "proto_man.h"


#include <iostream>
#include <string>


extern"C"
{
	static void 
	sv_send_cb(uv_udp_send_t* req, int status)
	{
		if (status==0)
		{

		}
		free(req);
	}
}


void 
UdpSession::Close()
{

}

const char* 
UdpSession::GetAddress(int* client_port)
{
	*client_port = this->c_port;
	return this->c_address;
}

void UdpSession::SendMsg(struct CmdMsg* msg)
{
	unsigned char* encode_pkg = NULL;
	int len1;
	encode_pkg = ProtoMan::EncodeMsgToRaw(msg, &len1);
	if (encode_pkg)
	{
		this->SendData(encode_pkg, len1);
		ProtoMan::MsgRawFree(encode_pkg);
	}
}


void
UdpSession::SendData(unsigned char* body, int len)
{
	uv_buf_t w_buf;
	w_buf = uv_buf_init((char*)body, len);
	uv_udp_send_t* req = (uv_udp_send_t*)malloc(sizeof(uv_udp_send_t));
	uv_udp_send(req, this->udp_handler, &w_buf, 1, this->addr, sv_send_cb);
}