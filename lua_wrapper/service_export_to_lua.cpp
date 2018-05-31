#include "lua_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "tolua++.h"
#ifdef __cplusplus
}
#endif

#include "tolua_fix.h"
#include "service_export_to_lua.h"
#include "../utils/logger.h"
#include "../netbus/service_man.h"
#include "../netbus/service.h"
#include "../netbus/proto_man.h"
#include "google\protobuf\message.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SERVICE_REFID_FUNCTION_MAPPING "service_refid_function_mapping"

using namespace google::protobuf;

static int s_function_ref_id = 0;

static
void init_service_function_map(lua_State* L) {
	lua_pushstring(L, SERVICE_REFID_FUNCTION_MAPPING);
	lua_newtable(L);
	lua_rawset(L, LUA_REGISTRYINDEX);
}


static int save_ref_function(lua_State* L, int lo, int def)
{
	// function at lo
	if (!lua_isfunction(L, lo)) return 0;

	s_function_ref_id++;

	lua_pushstring(L, SERVICE_REFID_FUNCTION_MAPPING);
	lua_rawget(L, LUA_REGISTRYINDEX);         /* stack: fun ... refid_fun */
	lua_pushinteger(L, s_function_ref_id);                      /* stack: fun ... refid_fun refid */
	lua_pushvalue(L, lo);                                       /* stack: fun ... refid_fun refid fun */
	lua_rawset(L, -3);     /* refid_fun[refid] = fun, stack: fun ... refid_ptr */
	lua_pop(L, 1);                                              /* stack: fun ... */
	return s_function_ref_id;

	// lua_pushvalue(L, lo);                                           /* stack: ... func */
	// return luaL_ref(L, LUA_REGISTRYINDEX);
}

static void
get_service_function(lua_State* L, int refid)
{
	lua_pushstring(L, SERVICE_REFID_FUNCTION_MAPPING);
	lua_rawget(L, LUA_REGISTRYINDEX);                           /* stack: ... refid_fun */
	lua_pushinteger(L, refid);                                  /* stack: ... refid_fun refid */
	lua_rawget(L, -2);                                          /* stack: ... refid_fun fun */
	lua_remove(L, -2);                                          /* stack: ... fun */
}

static bool
push_service_function(int nHandler)
{
	get_service_function(LuaWrapper::GetLuaState(), nHandler);                  /* L: ... func */
	if (!lua_isfunction(LuaWrapper::GetLuaState(), -1))
	{
		log_error("[LUA ERROR] function refid '%d' does not reference a Lua function", nHandler);
		lua_pop(LuaWrapper::GetLuaState(), 1);
		return false;
	}
	return true;
}

static int
exe_function(int numArgs)
{
	int functionIndex = -(numArgs + 1);
	if (!lua_isfunction(LuaWrapper::GetLuaState(), functionIndex))
	{
		log_error("value at stack [%d] is not function", functionIndex);
		lua_pop(LuaWrapper::GetLuaState(), numArgs + 1); // remove function and arguments
		return 0;
	}

	int traceback = 0;
	lua_getglobal(LuaWrapper::GetLuaState(), "__G__TRACKBACK__");                         /* L: ... func arg1 arg2 ... G */
	if (!lua_isfunction(LuaWrapper::GetLuaState(), -1))
	{
		lua_pop(LuaWrapper::GetLuaState(), 1);                                            /* L: ... func arg1 arg2 ... */
	}
	else
	{
		lua_insert(LuaWrapper::GetLuaState(), functionIndex - 1);                         /* L: ... G func arg1 arg2 ... */
		traceback = functionIndex - 1;
	}

	int error = 0;
	error = lua_pcall(LuaWrapper::GetLuaState(), numArgs, 1, traceback);                  /* L: ... [G] ret */
	if (error)
	{
		if (traceback == 0)
		{
			log_error("[LUA ERROR] %s", lua_tostring(LuaWrapper::GetLuaState(), -1));        /* L: ... error */
			lua_pop(LuaWrapper::GetLuaState(), 1); // remove error msg from stack
		}
		else                                                            /* L: ... G error */
		{
			lua_pop(LuaWrapper::GetLuaState(), 2); // remove __G__TRACKBACK__ and error msg from stack
		}
		return 0;
	}

	// get return value
	int ret = 0;
	if (lua_isnumber(LuaWrapper::GetLuaState(), -1))
	{
		ret = (int)lua_tointeger(LuaWrapper::GetLuaState(), -1);
	}
	else if (lua_isboolean(LuaWrapper::GetLuaState(), -1))
	{
		ret = (int)lua_toboolean(LuaWrapper::GetLuaState(), -1);
	}
	// remove return value from stack
	lua_pop(LuaWrapper::GetLuaState(), 1);                                                /* L: ... [G] */

	if (traceback)
	{
		lua_pop(LuaWrapper::GetLuaState(), 1); // remove __G__TRACKBACK__ from stack      /* L: ... */
	}

	return ret;
}

static int
execute_service_function(int nHandler, int numArgs) {
	int ret = 0;
	if (push_service_function(nHandler))                                /* L: ... arg1 arg2 ... func */
	{
		if (numArgs > 0)
		{
			lua_insert(LuaWrapper::GetLuaState(), -(numArgs + 1));                        /* L: ... func arg1 arg2 ... */
		}
		ret = exe_function(numArgs);
	}
	lua_settop(LuaWrapper::GetLuaState(), 0);
	return ret;
}





class LuaService :public Service
{
public:
	virtual bool OnSessionRecvCmd(Session* s, struct CmdMsg* msg);
	virtual void OnSessionDisconnect(Session* s);
public:
	unsigned int OnSessionRecvCmdHandler;
	unsigned int OnSessionDisconnectHandler;
};

static void push_proto_message_tolua(const Message* msg)
{
	lua_State* state = LuaWrapper::GetLuaState();
	if (!msg) {
		// printf("PushProtobuf2LuaTable failed, msg is NULL");
		return;
	}
	const Reflection* reflection = msg->GetReflection();

	// 顶层table
	lua_newtable(state);

	const Descriptor* descriptor = msg->GetDescriptor();
	for (int32_t index = 0; index < descriptor->field_count(); ++index) {
		const FieldDescriptor* fd = descriptor->field(index);
		const std::string& name = fd->lowercase_name();

		// key
		lua_pushstring(state, name.c_str());

		bool bReapeted = fd->is_repeated();

		if (bReapeted) {
			// repeated这层的table
			lua_newtable(state);
			int size = reflection->FieldSize(*msg, fd);
			for (int i = 0; i < size; ++i) {
				char str[32] = { 0 };
				switch (fd->cpp_type()) {
				case FieldDescriptor::CPPTYPE_DOUBLE:
					lua_pushnumber(state, reflection->GetRepeatedDouble(*msg, fd, i));
					break;
				case FieldDescriptor::CPPTYPE_FLOAT:
					lua_pushnumber(state, (double)reflection->GetRepeatedFloat(*msg, fd, i));
					break;
				case FieldDescriptor::CPPTYPE_INT64:
					sprintf(str, "%lld", (long long)reflection->GetRepeatedInt64(*msg, fd, i));
					lua_pushstring(state, str);
					break;
				case FieldDescriptor::CPPTYPE_UINT64:

					sprintf(str, "%llu", (unsigned long long)reflection->GetRepeatedUInt64(*msg, fd, i));
					lua_pushstring(state, str);
					break;
				case FieldDescriptor::CPPTYPE_ENUM: // 与int32一样处理
					lua_pushinteger(state, reflection->GetRepeatedEnum(*msg, fd, i)->number());
					break;
				case FieldDescriptor::CPPTYPE_INT32:
					lua_pushinteger(state, reflection->GetRepeatedInt32(*msg, fd, i));
					break;
				case FieldDescriptor::CPPTYPE_UINT32:
					lua_pushinteger(state, reflection->GetRepeatedUInt32(*msg, fd, i));
					break;
				case FieldDescriptor::CPPTYPE_STRING:
				{
					std::string value = reflection->GetRepeatedString(*msg, fd, i);
					lua_pushlstring(state, value.c_str(), value.size());
				}
				break;
				case FieldDescriptor::CPPTYPE_BOOL:
					lua_pushboolean(state, reflection->GetRepeatedBool(*msg, fd, i));
					break;
				case FieldDescriptor::CPPTYPE_MESSAGE:
					push_proto_message_tolua(&(reflection->GetRepeatedMessage(*msg, fd, i)));
					break;
				default:
					break;
				}

				lua_rawseti(state, -2, i + 1); // lua's index start at 1
			}

		}
		else {
			char str[32] = { 0 };
			switch (fd->cpp_type()) {

			case FieldDescriptor::CPPTYPE_DOUBLE:
				lua_pushnumber(state, reflection->GetDouble(*msg, fd));
				break;
			case FieldDescriptor::CPPTYPE_FLOAT:
				lua_pushnumber(state, (double)reflection->GetFloat(*msg, fd));
				break;
			case FieldDescriptor::CPPTYPE_INT64:

				sprintf(str, "%lld", (long long)reflection->GetInt64(*msg, fd));
				lua_pushstring(state, str);
				break;
			case FieldDescriptor::CPPTYPE_UINT64:

				sprintf(str, "%llu", (unsigned long long)reflection->GetUInt64(*msg, fd));
				lua_pushstring(state, str);
				break;
			case FieldDescriptor::CPPTYPE_ENUM: // 与int32一样处理
				lua_pushinteger(state, (int)reflection->GetEnum(*msg, fd)->number());
				break;
			case FieldDescriptor::CPPTYPE_INT32:
				lua_pushinteger(state, reflection->GetInt32(*msg, fd));
				break;
			case FieldDescriptor::CPPTYPE_UINT32:
				lua_pushinteger(state, reflection->GetUInt32(*msg, fd));
				break;
			case FieldDescriptor::CPPTYPE_STRING:
			{
				std::string value = reflection->GetString(*msg, fd);
				lua_pushlstring(state, value.c_str(), value.size());
			}
			break;
			case FieldDescriptor::CPPTYPE_BOOL:
				lua_pushboolean(state, reflection->GetBool(*msg, fd));
				break;
			case FieldDescriptor::CPPTYPE_MESSAGE:
				push_proto_message_tolua(&(reflection->GetMessage(*msg, fd)));
				break;
			default:
				break;
			}
		}

		lua_rawset(state, -3);
	}

}


bool LuaService::OnSessionRecvCmd(Session* s, struct CmdMsg* msg)
{
	static int index = 1;
	tolua_pushuserdata(LuaWrapper::GetLuaState(), (void*)s);
	//protobuf msg needs to convert to a lua table
	lua_newtable(LuaWrapper::GetLuaState());
	lua_pushinteger(LuaWrapper::GetLuaState(),msg->stype);
	lua_rawseti(LuaWrapper::GetLuaState(), -2, index);
	index++;
	lua_pushinteger(LuaWrapper::GetLuaState(), msg->ctype);
	lua_rawseti(LuaWrapper::GetLuaState(), -2, index);
	index++;
	lua_pushinteger(LuaWrapper::GetLuaState(), msg->utag);
	lua_rawseti(LuaWrapper::GetLuaState(), -2, index);
	index++;

	if (!msg->body)
	{
		lua_pushnil(LuaWrapper::GetLuaState());
	}
	else
	{
		if (ProtoMan::ProtoType()==PROTO_JSON)
		{
			lua_pushstring(LuaWrapper::GetLuaState(), (const char*)msg->body);
			
		}
		else
		{
			push_proto_message_tolua((const google::protobuf::Message*)msg->body);
		}
		lua_rawseti(LuaWrapper::GetLuaState(), -2, index);
		index++;
	}

	execute_service_function(this->OnSessionRecvCmdHandler, 2);
	return true;
}

void LuaService::OnSessionDisconnect(Session* s)
{
	tolua_pushuserdata(LuaWrapper::GetLuaState(), (void*)s);
	execute_service_function(this->OnSessionDisconnectHandler, 1);
}


static int
lua_service_register(lua_State *ls)
{
	int stype;
	stype = (int)tolua_tonumber(ls, 1, NULL);

	if (lua_istable(ls, 2))
	{
		unsigned int lua_recv_cmd_handler;
		unsigned int lua_disconnect_handler;

		lua_getfield(ls, 2, "on_session_recv_cmd");
		lua_getfield(ls, 2, "on_session_disconnect");

		lua_recv_cmd_handler = save_ref_function(ls, -2, NULL);
		lua_disconnect_handler = save_ref_function(ls, -1, NULL);

		if (lua_recv_cmd_handler == 0 || lua_disconnect_handler == 0)
		{
			return 1;
		}
		bool ret;
		LuaService* s = new LuaService();
		s->OnSessionDisconnectHandler = lua_disconnect_handler;
		s->OnSessionRecvCmdHandler = lua_recv_cmd_handler;
		ret = ServiceMan::RegisterService(stype, s);
		lua_pushnumber(ls, ret);
		return 1;
	}
	return 0;


}


int RegisterServiceExport(lua_State* L)
{
	init_service_function_map(L);
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1)) {
		tolua_open(L);
		tolua_module(L, "Service", 0);
		tolua_beginmodule(L, "Service");

		tolua_function(L, "RegisterService", lua_service_register);
		//tolua_function(L, "Close", lua_redis_close);
		//tolua_function(L, "Query", lua_redis_query);
		tolua_endmodule(L);
	}
	lua_pop(L, 1);
	return 0;
}