#ifndef __SCHEDULER_EXPORT_TO_LUA_H__
#define __SCHEDULER_EXPORT_TO_LUA_H__
struct lua_State;

int RegisterSchedulerExport(lua_State* L);

#endif // !__SERVICE_EXPORT_TO_LUA_H__
