#include "uv.h"
#include "../../netbus/netbus.h"
#include "../../netbus/proto_man.h"
#include "proto\pf_cmd_map.h"
#include "../../utils/logger.h"
#include "../../utils/time_list.h"
#include "../../utils/timestamp.h"
#include "../../database/mysql_wrapper.h"
#include "../../database/redis_wrapper.h"
#include "../../../lua_wrapper/lua_wrapper.h"

#include <iostream>
#include <string>


static void 
on_logger_timer(void* udata) 
{
	log_debug("on_logger_timer");
}
static void
on_query_cb(const char* err, MYSQL_RES* result,void* udata)
{
 	if (err != NULL)
	{
		printf("%s\n", err);
		return;
	}
	printf("query success\n");

}

 //void on_query_cb(const char* err, redisReply* replay)
 //{
	// if (err != NULL)
	// {
	//	 printf("³ö´í:%s\n", err);
	//	 return;
	// }
	// printf("query success\n");
 //}


//static void 
//on_open_cb(const char* err, void* context, void* udata)
//{
//	if (err!=NULL)
//	{
//		printf("³ö´í:%s\n", err);
//		return;
//	}
//	printf("connect success\n");
//	MysqlWrapper::Query(context,"select * from test",on_query_cb, udata);
//	//RedisWrapper::Close(context)
//}
//
//
//static void	
//TestDB()
//{
//	MysqlWrapper::Connect("127.0.0.1", 3306, "test", "root","123456", on_open_cb);
//	
//}
//static void
//TestRedis()
//{
//	RedisWrapper::Connect("127.0.0.1", 6379, on_open_cb);
//}


int main(int argc,char* argv[])
{
	//ProtoMan::Init(PROTO_BUF);
	//InitPFCmdMap();

	//log_debug("%d", timestamp());
	//log_debug("%d", timestamp_today());
	//log_debug("%d", date2timestamp("%Y-%m-%d %H:%M:%S", "2018-02-01 00:00:00"));

	//unsigned long yesterday = timestamp_yesterday();
	//char out_buf[64];
	//timestamp2date(yesterday, "%Y-%m-%d %H:%M:%S", out_buf, sizeof(out_buf));
	//log_debug("%s", out_buf);
	//schedule(on_logger_timer, NULL, 3000, -1);
	//TestRedis();
	logger::init("logger/gateway/", "gateway", true);
	LuaWrapper::LuaInit();
	LuaWrapper::LuaExec("./main.lua");
	Netbus *nb = Netbus::instance();
	nb->Init();
	nb->StartTcpServer(6080);
	nb->StartUdpServer(8002);
	nb->Run();

	LuaWrapper::LuaExit();

	return 0;
}
