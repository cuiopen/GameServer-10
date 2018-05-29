#ifndef __LUA_WRAPPER_H__
#define __LUA_WRAPPER_H__

#include "lua.hpp"

class LuaWrapper
{ 
public:
	static void LuaInit();
	static void LuaExit();

	static void LuaExec(const char* lua_path);
	static lua_State* GetLuaState();

public:
	static void RegFunc2lua(const char* name, int(*c_func)(lua_State *L));
public:
	static int ExecuteScriptHandler(int nHandler, int numArgs);
	static void RemoveScriptHandler(int nHandler);
};


#endif // !__LUA_WRAPPER_H__
