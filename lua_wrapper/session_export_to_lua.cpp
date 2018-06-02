#include "lua_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "tolua++.h"
#ifdef __cplusplus
}
#endif

#include "tolua_fix.h"
#include "../utils/logger.h"
#include "../netbus/service_man.h"
#include "../netbus/service.h"
#include "../netbus/proto_man.h"
#include "../netbus/session.h"
#include "session_export_to_lua.h"
#include "google\protobuf\message.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


using namespace google::protobuf;






static int
lua_Close_register(lua_State *ls)
{
	Session* s = (Session*)tolua_touserdata(ls, 1, NULL);
	if (s == NULL)
	{
		return 0;
	}
	s->Close();
	return 0;
}

static google::protobuf::Message*
lua_table_to_protobuf(lua_State *ls, int index, const char* msg_name)
{
	Message* msg = ProtoMan::create_message(msg_name);
	if (!msg)
	{
		log_error("cant find msg  %s from compiled poll \n", msg_name);
		return NULL;
	}

	const Reflection* reflection = msg->GetReflection();
	const Descriptor* descriptor = msg->GetDescriptor();

	// 遍历table的所有key， 并且与 protobuf结构相比较。如果require的字段没有赋值， 报错！ 如果找不到字段，报错！
	for (int32_t index = 0; index < descriptor->field_count(); ++index)
	{
		const FieldDescriptor* fd = descriptor->field(index);
		const string& name = fd->name();

		bool isRequired = fd->is_required();
		bool bReapeted = fd->is_repeated();
		lua_pushstring(ls, name.c_str());
		lua_rawget(ls, index);

		bool isNil = lua_isnil(ls, -1);

		if (bReapeted)
		{
			if (isNil)
			{
				lua_pop(ls, 1);
				continue;
			}
			else
			{
				bool isTable = lua_istable(ls, -1);
				if (!isTable)
				{
					log_error("cant find required repeated field %s\n", name.c_str());
					ProtoMan::release_message(msg);
					return NULL;
				}
			}

			lua_pushnil(ls);
			for (; lua_next(ls, -2) != 0;)
			{
				switch (fd->cpp_type())
				{

				case FieldDescriptor::CPPTYPE_DOUBLE:
				{
					double value = luaL_checknumber(ls, -1);
					reflection->AddDouble(msg, fd, value);
				}
				break;
				case FieldDescriptor::CPPTYPE_FLOAT:
				{
					float value = luaL_checknumber(ls, -1);
					reflection->AddFloat(msg, fd, value);
				}
				break;
				case FieldDescriptor::CPPTYPE_INT64:
				{
					int64_t value = luaL_checknumber(ls, -1);
					reflection->AddInt64(msg, fd, value);
				}
				break;
				case FieldDescriptor::CPPTYPE_UINT64:
				{
					uint64_t value = luaL_checknumber(ls, -1);
					reflection->AddUInt64(msg, fd, value);
				}
				break;
				case FieldDescriptor::CPPTYPE_ENUM: // 与int32一样处理
				{
					int32_t value = luaL_checknumber(ls, -1);
					const EnumDescriptor* enumDescriptor = fd->enum_type();
					const EnumValueDescriptor* valueDescriptor = enumDescriptor->FindValueByNumber(value);
					reflection->AddEnum(msg, fd, valueDescriptor);
				}
				break;
				case FieldDescriptor::CPPTYPE_INT32:
				{
					int32_t value = luaL_checknumber(ls, -1);
					reflection->AddInt32(msg, fd, value);
				}
				break;
				case FieldDescriptor::CPPTYPE_UINT32:
				{
					uint32_t value = luaL_checknumber(ls, -1);
					reflection->AddUInt32(msg, fd, value);
				}
				break;
				case FieldDescriptor::CPPTYPE_STRING:
				{
					size_t size = 0;
					const char* value = luaL_checklstring(ls, -1, &size);
					reflection->AddString(msg, fd, std::string(value, size));
				}
				break;
				case FieldDescriptor::CPPTYPE_BOOL:
				{
					bool value = lua_toboolean(ls, -1);
					reflection->AddBool(msg, fd, value);
				}
				break;
				case FieldDescriptor::CPPTYPE_MESSAGE:
				{
					Message* value = lua_table_to_protobuf(ls, lua_gettop(ls), fd->message_type()->name().c_str());
					if (!value)
					{
						log_error("convert to msg %s failed whith value %s\n", fd->message_type()->name().c_str(), name.c_str());
						ProtoMan::release_message(value);
						return NULL;
					}
					Message* msg = reflection->AddMessage(msg, fd);
					msg->CopyFrom(*value);
					ProtoMan::release_message(value);
				}
				break;
				default:
					break;
				}

				// remove value， keep the key
				lua_pop(ls, 1);
			}
		}
		else
		{

			if (isRequired)
			{
				if (isNil)
				{
					log_error("cant find required field %s\n", name.c_str());
					ProtoMan::release_message(msg);
					return NULL;
				}
			}
			else
			{
				if (isNil)
				{
					lua_pop(ls, 1);
					continue;
				}
			}

			switch (fd->cpp_type())
			{
			case FieldDescriptor::CPPTYPE_DOUBLE:
			{
				double value = luaL_checknumber(ls, -1);
				reflection->SetDouble(msg, fd, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_FLOAT:
			{
				float value = luaL_checknumber(ls, -1);
				reflection->SetFloat(msg, fd, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_INT64:
			{
				int64_t value = luaL_checknumber(ls, -1);
				reflection->SetInt64(msg, fd, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_UINT64:
			{
				uint64_t value = luaL_checknumber(ls, -1);
				reflection->SetUInt64(msg, fd, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_ENUM: // 与int32一样处理
			{
				int32_t value = luaL_checknumber(ls, -1);
				const EnumDescriptor* enumDescriptor = fd->enum_type();
				const EnumValueDescriptor* valueDescriptor = enumDescriptor->FindValueByNumber(value);
				reflection->SetEnum(msg, fd, valueDescriptor);
			}
			break;
			case FieldDescriptor::CPPTYPE_INT32:
			{
				int32_t value = luaL_checknumber(ls, -1);
				reflection->SetInt32(msg, fd, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_UINT32:
			{
				uint32_t value = luaL_checknumber(ls, -1);
				reflection->SetUInt32(msg, fd, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_STRING:
			{
				size_t size = 0;
				const char* value = luaL_checklstring(ls, -1, &size);
				reflection->SetString(msg, fd, std::string(value, size));
			}
			break;
			case FieldDescriptor::CPPTYPE_BOOL:
			{
				bool value = lua_toboolean(ls, -1);
				reflection->SetBool(msg, fd, value);
			}
			break;
			case FieldDescriptor::CPPTYPE_MESSAGE:
			{
				Message* value = lua_table_to_protobuf(ls, lua_gettop(ls), fd->message_type()->name().c_str());
				if (!value)
				{
					log_error("convert to msg %s failed whith value %s \n", fd->message_type()->name().c_str(), name.c_str());
					ProtoMan::release_message(msg);
					return NULL;
				}
				Message* msg = reflection->MutableMessage(msg, fd);
				msg->CopyFrom(*value);
				ProtoMan::release_message(value);
			}
			break;
			default:
				break;
			}
		}

		// pop value
		lua_pop(ls, 1);
	}

	return msg;

}



static int
lua_SendMsgn_register(lua_State *ls)
{
	Session* s = (Session*)tolua_touserdata(ls, 1, NULL);
	if (s == NULL)
	{
		return 0;
	}
	if (lua_istable(ls, 2))
	{
		return 0;
	}
	lua_getfield(ls, 2, "1");
	lua_getfield(ls, 2, "2");
	lua_getfield(ls, 2, "3");
	lua_getfield(ls, 2, "4");
	CmdMsg msg;
	msg.stype = lua_tointeger(ls, 3);
	msg.ctype = lua_tointeger(ls, 4);
	msg.utag = lua_tointeger(ls, 5);
	if (ProtoMan::ProtoType() == PROTO_JSON)
	{
		msg.body = (char*)lua_tostring(ls, 6);
	}
	else
	{
		if (!lua_istable(ls, 6))
		{
			msg.body = NULL;
		}
		else
		{
			const char* msg_name = ProtoMan::CtypeToName(msg.ctype);
			msg.body = lua_table_to_protobuf(ls, 6, msg_name);
		}
	}
	s->SendMsg(&msg);
	ProtoMan::release_message((google::protobuf::Message*)msg.body);
	return 0;
}


static int
lua_GetAddress_register(lua_State *ls)
{
	Session* s = (Session*)tolua_touserdata(ls, 1, NULL);
	if (s == NULL)
	{
		return 0;
	}
	int port;
	const char* ip = s->GetAddress(&port);
	lua_pushstring(ls, ip);
	lua_pushinteger(ls, port);
	return 2;
}



int RegisterSessionExport(lua_State* L)
{
	lua_getglobal(L, "_G");
	if (lua_istable(L, -1)) {
		tolua_open(L);
		tolua_module(L, "Session", 0);
		tolua_beginmodule(L, "Session");

		tolua_function(L, "Close", lua_Close_register);
		tolua_function(L, "SendMsg", lua_SendMsgn_register);
		tolua_function(L, "GetAddress", lua_GetAddress_register);
		tolua_endmodule(L);
	}
	lua_pop(L, 1);
	return 0;
}