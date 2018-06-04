#include "lua_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "tolua++.h"
#ifdef __cplusplus
}
#endif

#include "tolua_fix.h"
#include "../utils/time_list.h"
#include "../utils/logger.h"
#include "scheduler_export_to_lua.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>



struct timer_repeat
{
	int handler;
	int repeat_count;
};


static void on_repeat_timer(void* udata)
{
	timer_repeat* tr = (timer_repeat*)udata;
	LuaWrapper::ExecuteScriptHandler(tr->handler, 0);
	if (tr->repeat_count==-1)
	{
		return;
	}
	tr->repeat_count--;
	if (tr->repeat_count<=0)
	{
		LuaWrapper::RemoveScriptHandler(tr->handler);
		free(tr);
	}

}

//static void on_once_timer(void* udata)
//{
//	timer_repeat* tr = (timer_repeat*)udata;
//	LuaWrapper::ExecuteScriptHandler(tr->handler, 0);
//	LuaWrapper::RemoveScriptHandler(tr->handler);
//	free(tr);
//}

static int
lua_schedule_register(lua_State* L)
{
	int handler = toluafix_ref_function(L, 1, 0);
	if (handler==0)
	{
		goto x1;
	}
	int after_msec = lua_tointeger(L, 2, 0);
	if (after_msec<=0)
	{
		goto x1;
	}
	int repeat_count = lua_tointeger(L, 3, 0);
	if (repeat_count == 0)
	{
		goto x1;
	}
	if (repeat_count < 0)
	{
		repeat_count = -1;
	}

	int repeat_msec = lua_tointeger(L, 4, 0);
	if (repeat_msec <= 0)
	{
		repeat_msec= after_msec;
	}
	
	timer_repeat* tr = (timer_repeat*)malloc(sizeof(timer_repeat));
	tr->handler = handler;
	tr->repeat_count = repeat_count;
	timer* t = schedule_repeat(on_repeat_timer,(void*)tr,after_msec,repeat_count,repeat_msec);
	tolua_pushuserdata(L, t);
	return 1;
x1:
	if (handler!=0)
	{
		LuaWrapper::RemoveScriptHandler(handler);
	}
	lua_pushnil(L);
	return 1;
}

static int
lua_cancel_register(lua_State* L)
{
	if (!lua_isuserdata(L,1))
	{
		return 0;
	}

	timer* t = (timer*)lua_touserdata(L, 1);
	timer_repeat* tr = (timer_repeat*)get_timer_udata(t);
	LuaWrapper::RemoveScriptHandler(tr->handler);
	free(tr);
	cancel_timer(t);
	return 0;
}

static int
lua_once_register(lua_State* L)
{
	int handler = toluafix_ref_function(L, 1, 0);
	if (handler == 0)
	{
		goto x1;
	}
	int after_msec = lua_tointeger(L, 2, 0);
	if (after_msec <= 0)
	{
		goto x1;
	}
	timer_repeat* tr = (timer_repeat*)malloc(sizeof(timer_repeat));
	tr->handler = handler;
	tr->repeat_count = 1;
	timer* t = schedule_once(on_repeat_timer,(void*)tr, after_msec);
	tolua_pushuserdata(L, t);
	return 1;

x1:
	if (handler != 0)
	{
		LuaWrapper::RemoveScriptHandler(handler);
	}
	lua_pushnil(L);
	return 1;
}




int RegisterSchedulerExport(lua_State* L)
{
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1)) {
		tolua_open(L);
		tolua_module(L, "Scheduler", 0);
		tolua_beginmodule(L, "Scheduler");
		tolua_function(L, "Schedule", lua_schedule_register);
		tolua_function(L, "Cancel", lua_cancel_register);
		tolua_function(L, "Once", lua_once_register);
		tolua_endmodule(L);
	}
	lua_pop(L, 1);
	return 0;
}