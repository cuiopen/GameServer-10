#include "uv.h"
#include "../../netbus/netbus.h"
#include "../../netbus/proto_man.h"
#include "proto\pf_cmd_map.h"
#include "../../utils/logger.h"
#include "../../utils/time_list.h"
#include "../../utils/timestamp.h"

#include <iostream>
#include <string>


static
void on_logger_timer(void* udata) {
	log_debug("on_logger_timer");
}




int main(int argc,char* argv[])
{
	//ProtoMan::Init(PROTO_BUF);
	//InitPFCmdMap();
	//logger::init("logger/gateway/", "gateway", true);

	//log_debug("%d", timestamp());
	//log_debug("%d", timestamp_today());
	//log_debug("%d", date2timestamp("%Y-%m-%d %H:%M:%S", "2018-02-01 00:00:00"));


	//unsigned long yesterday = timestamp_yesterday();
	//char out_buf[64];
	//timestamp2date(yesterday, "%Y-%m-%d %H:%M:%S", out_buf, sizeof(out_buf));
	//log_debug("%s", out_buf);
	//schedule(on_logger_timer, NULL, 3000, -1);
	Netbus *nb = Netbus::instance();
	nb->Init();
	nb->StartTcpServer(6080);
	nb->StartUdpServer(8002);
	nb->Run();

	return 0;
}
