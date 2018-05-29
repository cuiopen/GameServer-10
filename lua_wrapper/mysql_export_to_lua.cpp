#include "lua_wrapper.h"
#include "../database/mysql_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "tolua++.h"
#ifdef __cplusplus
}
#endif

#include "tolua_fix.h"
#include "mysql_export_to_lua.h"
#include "../utils/logger.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void
on_open_cb(const char* err, void* context, void* udata) 
{
	if (err) 
	{
		lua_pushstring(LuaWrapper::GetLuaState(), err);
		lua_pushnil(LuaWrapper::GetLuaState());
	}
	else 
	{
		lua_pushnil(LuaWrapper::GetLuaState());
		tolua_pushuserdata(LuaWrapper::GetLuaState(), context);
	}

	LuaWrapper::ExecuteScriptHandler((int)udata,2);
	LuaWrapper::RemoveScriptHandler((int)udata);
}

static void
push_mysql_row(MYSQL_ROW row, int num)
{
	lua_newtable(LuaWrapper::GetLuaState());                                              /* L: table */
	int index = 1;
	for (int i = 0; i < num; i++) 
	{
		if (row[i] == NULL) 
		{
			lua_pushnil(LuaWrapper::GetLuaState());
		}
		else {
			lua_pushstring(LuaWrapper::GetLuaState(), row[i]);
		}

		lua_rawseti(LuaWrapper::GetLuaState(), -2, index);          /* table[index] = value, L: table */
		++index;
	}
}

static void
on_query_cb(const char* err, MYSQL_RES* result, void* udata)
{
	if (err)
	{
		lua_pushstring(LuaWrapper::GetLuaState(), err);
		lua_pushnil(LuaWrapper::GetLuaState());
	}
	else
	{
		lua_pushnil(LuaWrapper::GetLuaState());
		if (result)
		{ // 把查询得到的结果push成一个表; { {}, {}, {}, ...}
			lua_newtable(LuaWrapper::GetLuaState());
			int index = 1;
			int num = mysql_num_fields(result);
			MYSQL_ROW row;
			while (row = mysql_fetch_row(result)) 
			{
				push_mysql_row(row, num); /* L: table value */
				lua_rawseti(LuaWrapper::GetLuaState(), -2, index);          /* table[index] = value, L: table */
				++index;
			}
		}
		else
		{
			lua_pushnil(LuaWrapper::GetLuaState());
		}
		LuaWrapper::ExecuteScriptHandler((int)udata, 2);
		LuaWrapper::RemoveScriptHandler((int)udata);
	}
}

int lua_mysql_connect(lua_State* L)
{
	char* ip = (char*)tolua_tostring(L, 1, NULL);
	if (ip==NULL)
	{
		log_error("can't read ip");
	}

	int port = (int)tolua_tonumber(L, 2, NULL);

	char* db_name = (char*)tolua_tostring(L, 3, NULL);
	if (db_name == NULL) {
		log_error("can't read db_name");
	}

	char* uname = (char*)tolua_tostring(L, 4, NULL);
	if (uname == NULL) {
		log_error("can't read uname");
	}

	char* upwd = (char*)tolua_tostring(L, 5, NULL);
	if (upwd == NULL) {
		log_error("can't read upwd");
	}

	int handler = toluafix_ref_function(L, 6, NULL);
	MysqlWrapper::Connect(ip, port, db_name, uname, upwd, on_open_cb, (void*)handler);

	return 0;
}
int lua_mysql_close(lua_State* L)
{
	void* context = tolua_touserdata(LuaWrapper::GetLuaState(), 1, 0);
	if (context) {
		MysqlWrapper::Close(context);
	}

	return 0;
}

int lua_mysql_query(lua_State* L)
{
	void* context = tolua_touserdata(LuaWrapper::GetLuaState(), 1, 0);
	if (!context) 
	{
		return 1;
	}
	char* sql = (char*)tolua_tostring(L, 2, NULL);
	if (sql == NULL) 
	{
		return 1;
	}
	int handler = toluafix_ref_function(L, 3, NULL);
	if (handler==0)
	{
		return 1;
	}

	MysqlWrapper::Query(context, sql, on_query_cb, (void*)handler);
	return 0;
}




int RegisterMysqlExport(lua_State* L)
{
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1)) {
		tolua_open(L);
		tolua_module(L, "MysqlWrapper", 0);
		tolua_beginmodule(L, "MysqlWrapper");

		tolua_function(L, "Connect", lua_mysql_connect);
		tolua_function(L, "Close", lua_mysql_close);
		tolua_function(L, "Query", lua_mysql_query);
		tolua_endmodule(L);
	}
	lua_pop(L, 1);
	return 0;
}