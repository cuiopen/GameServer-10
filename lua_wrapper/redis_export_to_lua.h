#ifndef __Redis_EXPORT_TO_LUA_H__
#define __Redis_EXPORT_TO_LUA_H__

struct lua_State;
int RegisterRedisExport(lua_State* L);

#endif