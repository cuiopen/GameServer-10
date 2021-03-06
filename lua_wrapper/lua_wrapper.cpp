#include "lua_wrapper.h"
#include "lua.hpp"
#include "../utils/logger.h"
#include "tolua++.h"
#include "tolua_fix.h"
#include "mysql_export_to_lua.h"
#include "redis_export_to_lua.h"
#include "service_export_to_lua.h"
#include "session_export_to_lua.h"
#include "scheduler_export_to_lua.h"

#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>  

static lua_State* ls = NULL;
extern "C"
{
	static void
	log_DBUG(const char* file_name, int line_num, const char* msg)
	{
		logger::log(file_name, line_num, DEBUG, msg);
	}

	static void
	log_WARNING(const char* file_name, int line_num, const char* msg)
	{
		logger::log(file_name, line_num, WARNING, msg);
	}

	static void
	log_ERROR(const char* file_name, int line_num, const char* msg)
	{
		logger::log(file_name, line_num, SERIOUS, msg);
	}



	static void
	do_log_message(void(*slog)(const char* file_name, int line_num, const char* msg), const char* msg)
	{
		lua_Debug info;
		int depth = 0;
		while (lua_getstack(ls, depth, &info)) {

			lua_getinfo(ls, "S", &info);
			lua_getinfo(ls, "n", &info);
			lua_getinfo(ls, "l", &info);

			if (info.source[0] == '@') {
				slog(&info.source[1], info.currentline, msg);
				return;
			}

			++depth;
		}
		if (depth == 0) {
			slog("trunk", 0, msg);
		}
	}
	static int
	lua_log_debug(lua_State *luastate)
	{
		int nargs = lua_gettop(luastate);
		std::string t;
		for (int i = 1; i <= nargs; i++)
		{
			if (lua_istable(luastate, i))
				t += "table";
			else if (lua_isnone(luastate, i))
				t += "none";
			else if (lua_isnil(luastate, i))
				t += "nil";
			else if (lua_isboolean(luastate, i))
			{
				if (lua_toboolean(luastate, i) != 0)
					t += "true";
				else
					t += "false";
			}
			else if (lua_isfunction(luastate, i))
				t += "function";
			else if (lua_islightuserdata(luastate, i))
				t += "lightuserdata";
			else if (lua_isthread(luastate, i))
				t += "thread";
			else
			{
				const char * str = lua_tostring(luastate, i);
				if (str)
					t += lua_tostring(luastate, i);
				else
					t += lua_typename(luastate, lua_type(luastate, i));
			}
			if (i != nargs)
				t += "\t";
		}
		do_log_message(log_DBUG, t.c_str());
		return 0;
	}

	static int
	lua_log_warning(lua_State *luastate)
	{
		int nargs = lua_gettop(luastate);
		std::string t;
		for (int i = 1; i <= nargs; i++)
		{
			if (lua_istable(luastate, i))
				t += "table";
			else if (lua_isnone(luastate, i))
				t += "none";
			else if (lua_isnil(luastate, i))
				t += "nil";
			else if (lua_isboolean(luastate, i))
			{
				if (lua_toboolean(luastate, i) != 0)
					t += "true";
				else
					t += "false";
			}
			else if (lua_isfunction(luastate, i))
				t += "function";
			else if (lua_islightuserdata(luastate, i))
				t += "lightuserdata";
			else if (lua_isthread(luastate, i))
				t += "thread";
			else
			{
				const char * str = lua_tostring(luastate, i);
				if (str)
					t += lua_tostring(luastate, i);
				else
					t += lua_typename(luastate, lua_type(luastate, i));
			}
			if (i != nargs)
				t += "\t";
		}
		do_log_message(log_WARNING, t.c_str());
		return 0;
	}

	static int
	lua_log_error(lua_State *luastate)
	{
		int nargs = lua_gettop(luastate);
		std::string t;
		for (int i = 1; i <= nargs; i++)
		{
			if (lua_istable(luastate, i))
				t += "table";
			else if (lua_isnone(luastate, i))
				t += "none";
			else if (lua_isnil(luastate, i))
				t += "nil";
			else if (lua_isboolean(luastate, i))
			{
				if (lua_toboolean(luastate, i) != 0)
					t += "true";
				else
					t += "false";
			}
			else if (lua_isfunction(luastate, i))
				t += "function";
			else if (lua_islightuserdata(luastate, i))
				t += "lightuserdata";
			else if (lua_isthread(luastate, i))
				t += "thread";
			else
			{
				const char * str = lua_tostring(luastate, i);
				if (str)
					t += lua_tostring(luastate, i);
				else
					t += lua_typename(luastate, lua_type(luastate, i));
			}
			if (i != nargs)
				t += "\t";
		}
		do_log_message(log_ERROR, t.c_str());
		return 0;
	}

	static int
	lua_panic(lua_State *L) 
	{
		const char* msg = luaL_checkstring(L, -1);
		if (msg) { // file_name, line_num
			do_log_message(log_ERROR, msg);
		}
		return 0;
	}
}


lua_State* LuaWrapper::GetLuaState()
{
	return ls;
}

static int
lua_log_init(lua_State* ls)
{
	const char* path = lua_tostring(ls, 1);
	if (path==NULL)
	{
		return 0;
	}
	const char* prefix = lua_tostring(ls, 2);
	if (prefix==NULL)
	{
		return 0;
	}
	bool std_out = lua_toboolean(ls, 3);
	logger::init((char*)path, (char*)prefix, std_out);
}


static int
RegisterLoggerExport(lua_State* ls)
{
	LuaWrapper::RegFunc2lua("print", lua_log_debug);
	lua_getglobal(ls, "_G");
	if (lua_istable(ls, -1)) {
		tolua_open(ls);
		tolua_module(ls, "Logger", 0);
		tolua_beginmodule(ls, "Logger");

		tolua_function(ls, "Debug", lua_log_debug);
		tolua_function(ls, "Warning", lua_log_warning);
		tolua_function(ls, "Error", lua_log_error);
		tolua_function(ls, "Init", lua_log_init);
		tolua_endmodule(ls);
	}
	lua_pop(ls, 1);
	return 0;
}

void LuaWrapper::LuaInit()
{
	if (ls == NULL)
	{
		ls = luaL_newstate();
		luaL_openlibs(ls);
		lua_atpanic(ls, lua_panic);
		toluafix_open(ls);
		RegisterLoggerExport(ls);
		RegisterMysqlExport(ls);
		RegisterRedisExport(ls);
		RegisterServiceExport(ls);
		RegisterSessionExport(ls);
		RegisterSchedulerExport(ls);
	}
}
void LuaWrapper::LuaExit()
{
	if (ls != NULL)
	{
		lua_close(ls);
		ls = NULL;
	}
}
void LuaWrapper::LuaExec(const char* lua_path)
{
	if (luaL_dofile(ls, lua_path))
	{
		lua_log_error(ls);
		return;
	}
	return;
}


void LuaWrapper::RegFunc2lua(const char* name, int(*c_func)(lua_State *L))
{
	lua_pushcfunction(ls, c_func);
	lua_setglobal(ls, name);
}

static bool
pushFunctionByHandler(int nHandler)
{
	toluafix_get_function_by_refid(ls, nHandler);                  /* L: ... func */
	if (!lua_isfunction(ls, -1))
	{
		log_error("[LUA ERROR] function refid '%d' does not reference a Lua function", nHandler);
		lua_pop(ls, 1);
		return false;
	}
	return true;
}

static int
executeFunction(int numArgs)
{
	int functionIndex = -(numArgs + 1);
	if (!lua_isfunction(ls, functionIndex))
	{
		log_error("value at stack [%d] is not function", functionIndex);
		lua_pop(ls, numArgs + 1); // remove function and arguments
		return 0;
	}

	int traceback = 0;
	lua_getglobal(ls, "__G__TRACKBACK__");                         /* L: ... func arg1 arg2 ... G */
	if (!lua_isfunction(ls, -1))
	{
		lua_pop(ls, 1);                                            /* L: ... func arg1 arg2 ... */
	}
	else
	{
		lua_insert(ls, functionIndex - 1);                         /* L: ... G func arg1 arg2 ... */
		traceback = functionIndex - 1;
	}

	int error = 0;
	error = lua_pcall(ls, numArgs, 1, traceback);                  /* L: ... [G] ret */
	if (error)
	{
		if (traceback == 0)
		{
			log_error("[LUA ERROR] %s", lua_tostring(ls, -1));        /* L: ... error */
			lua_pop(ls, 1); // remove error message from stack
		}
		else                                                            /* L: ... G error */
		{
			lua_pop(ls, 2); // remove __G__TRACKBACK__ and error message from stack
		}
		return 0;
	}

	// get return value
	int ret = 0;
	if (lua_isnumber(ls, -1))
	{
		ret = (int)lua_tointeger(ls, -1);
	}
	else if (lua_isboolean(ls, -1))
	{
		ret = (int)lua_toboolean(ls, -1);
	}
	// remove return value from stack
	lua_pop(ls, 1);                                                /* L: ... [G] */

	if (traceback)
	{
		lua_pop(ls, 1); // remove __G__TRACKBACK__ from stack      /* L: ... */
	}

	return ret;
}


int
LuaWrapper::ExecuteScriptHandler(int nHandler, int numArgs) {
	int ret = 0;
	if (pushFunctionByHandler(nHandler))                                /* L: ... arg1 arg2 ... func */
	{
		if (numArgs > 0)
		{
			lua_insert(ls, -(numArgs + 1));                        /* L: ... func arg1 arg2 ... */
		}
		ret = executeFunction(numArgs);
	}
	lua_settop(ls, 0);
	return ret;
}

void
LuaWrapper::RemoveScriptHandler(int nHandler)
{
	toluafix_remove_function_by_refid(ls, nHandler);
}