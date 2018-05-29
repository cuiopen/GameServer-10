#ifndef __MYSQL_WRAPPER_H__
#define __MYSQL_WRAPPER_H__

#include "mysql.h"

class MysqlWrapper 
{
public:
	static void Connect(char* ip, int port,	char* db_name, char* uname, char* pwd, void(*open_cb)(const char* err, void* context, void* udata), void* udata = NULL);

	static void Close(void* context);

	static void Query(void * context, char * sql, void(*query_cb)(const char *err, MYSQL_RES *result, void* udata), void * udata=NULL);
};




#endif // !__MYSQL_WRAPPER_H__
