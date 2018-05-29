#include "lua_wrapper.h"
#include "../database/redis_wrapper.h"
#include "hiredis.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "tolua++.h"
#ifdef __cplusplus
}
#endif

#include "tolua_fix.h"
#include "redis_export_to_lua.h"
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
push_result_to_lua(redisReply* result)
{
	switch (result->type)
	{
	case REDIS_REPLY_STATUS:
	case REDIS_REPLY_STRING:
		lua_pushstring(LuaWrapper::GetLuaState(), result->str);
		break;
	case REDIS_REPLY_INTEGER:
		lua_pushinteger(LuaWrapper::GetLuaState(), result->integer);
		break;
	case REDIS_REPLY_NIL:
		lua_pushnil(LuaWrapper::GetLuaState());
		break;
	case REDIS_REPLY_ARRAY:
		lua_newtable(LuaWrapper::GetLuaState());
		int index = 1;
		for (int i = 0; i < result->elements; i++)
		{
			push_result_to_lua(result->element[i]);
			lua_rawseti(LuaWrapper::GetLuaState(), -2, index);          /* table[index] = value, L: table */
			++index;
		}
		break;
	}
}

static void
on_query_cb(redisReply* result, void* udata)
{

	if (result->type== REDIS_REPLY_ERROR)
	{
		lua_pushstring(LuaWrapper::GetLuaState(),result->str);
		lua_pushnil(LuaWrapper::GetLuaState());
	}
	else
	{
		lua_pushnil(LuaWrapper::GetLuaState());
		if (result)
		{ 
			push_result_to_lua(result);

		}
		else
		{
			lua_pushnil(LuaWrapper::GetLuaState());
		}
		LuaWrapper::ExecuteScriptHandler((int)udata, 2);
		LuaWrapper::RemoveScriptHandler((int)udata);
	}
}

int lua_redis_connect(lua_State* L)
{
	char* ip = (char*)tolua_tostring(L, 1, NULL);
	if (ip==NULL)
	{
		log_error("can't read ip");
		return 1;
	}

	int port = (int)tolua_tonumber(L, 2, NULL);
	if (port == NULL) {
		log_error("can't read PORT");
		return 1;
	}


	int handler = toluafix_ref_function(L,3, NULL);
	if (handler == NULL)
	{
		log_error("can't read FOO");
		return 1;
	}

	RedisWrapper::Connect(ip, port,on_open_cb, (void*)handler);

	return 0;
}

int lua_redis_close(lua_State* L)
{
	void* context = tolua_touserdata(LuaWrapper::GetLuaState(), 1, 0);
	if (context) {
		RedisWrapper::Close(context);
	}

	return 0;
}

int lua_redis_query(lua_State* L)
{
	void* context = tolua_touserdata(LuaWrapper::GetLuaState(), 1, 0);
	if (!context) 
	{
		log_error("can't read CMD object");
		return 1;
	}
	char* sql = (char*)tolua_tostring(L, 2, NULL);
	if (sql == NULL) 
	{
		log_error("can't read CMD");
		return 1;
	}
	int handler = toluafix_ref_function(L, 3, NULL);
	if (handler==NULL)
	{
		log_error("can't read FOO");
		return 1;
	}

	RedisWrapper::Query(context, sql, on_query_cb, (void*)handler);
	return 0;
}




int RegisterRedisExport(lua_State* L)
{
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1)) {
		tolua_open(L);
		tolua_module(L, "RedisWrapper", 0);
		tolua_beginmodule(L, "RedisWrapper");

		tolua_function(L, "Connect", lua_redis_connect);
		tolua_function(L, "Close", lua_redis_close);
		tolua_function(L, "Query", lua_redis_query);
		tolua_endmodule(L);
	}
	lua_pop(L, 1);
	return 0;
}