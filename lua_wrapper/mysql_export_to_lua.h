#ifndef __MYSQL_EXPORT_TO_LUA_H__
#define __MYSQL_EXPORT_TO_LUA_H__

struct lua_State;
int RegisterMysqlExport(lua_State* L);

#endif