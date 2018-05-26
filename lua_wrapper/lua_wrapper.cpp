#include "lua_wrapper.h"
#include "../3rd/lua-5.3.4/src/lua.hpp"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>  

static lua_State* ls=NULL;


void LuaWrapper::LuaInit()
{
	if (ls==NULL)
	{
		ls = luaL_newstate();
		luaL_openlibs(ls);
	}
}
void LuaWrapper::LuaExit()
{
	if (ls!=NULL)
	{
		lua_close(ls);
		ls = NULL;
	}
}
void LuaWrapper::LuaExec(const char* lua_path)
{
	assert(!luaL_dofile(ls, lua_path));
}
