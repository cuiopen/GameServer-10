#ifndef __REDIS_WRAPPER_H__
#define __REDIS_WRAPPER_H__
#include <vector>
#include <string>
class RedisWrapper 
{
public:
	static void Connect(char* ip, int port,void(*open_cb)(const char* err, void* context));

	static void Close(void* context);

	static void Query(void* context,char* sql,void(*query_cb)(const char* err, std::vector<std::vector<std::string>>* result));
};




#endif // !__MYSQL_WRAPPER_H__
