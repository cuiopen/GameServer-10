#ifndef __REDIS_WRAPPER_H__
#define __REDIS_WRAPPER_H__

struct redisReply;


class RedisWrapper 
{
public:
	static void Connect(char* ip, int port,void(*open_cb)(const char* err, void* context,void* udata), void* udata);

	static void Close(void* context);

	static void Query(void* context,char* cmd,void(*query_cb)(redisReply* replay, void* udata), void* udata);
};




#endif // !__MYSQL_WRAPPER_H__
