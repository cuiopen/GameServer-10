#ifndef __LUA_WRAPPER_H__
#define __LUA_WRAPPER_H__


class LuaWrapper
{ 
public:
	static void LuaInit();
	static void LuaExit();

	static void LuaExec(const char* lua_path);


};


#endif // !__LUA_WRAPPER_H__
