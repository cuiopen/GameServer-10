#include "proto_man.h"
#include "google\protobuf\message.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_PF_MAP_SIZE 1024
static int pro_type = PROTO_BUF;
static char* g_pf_map[MAX_PF_MAP_SIZE];
static int g_cmd_count = 0;

static google::protobuf::Message* create_message(const char* typeName) 
{
	google::protobuf::Message* message = NULL;
	const google::protobuf::Descriptor* descriptor = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(typeName);
	if (descriptor) {
		const google::protobuf::Message* prototype = google::protobuf::MessageFactory::generated_factory()->GetPrototype(descriptor);
		if (prototype) {
			message = prototype->New();
		}
	}
	return message;
}

static void
release_message(google::protobuf::Message* msg)
{
	delete msg;
}

void ProtoMan::Init(int proto_type)
{
	pro_type = proto_type;
}
void ProtoMan::RegisterPFCmdMap(char** pf_map, int len)
{
	len = (MAX_PF_MAP_SIZE - g_cmd_count) < len ? (MAX_PF_MAP_SIZE - g_cmd_count) : len;//丢掉超过最大值部分的命令

	
	for (int i = 0; i < len; i++)
	{
		g_pf_map[g_cmd_count + i] = strdup(pf_map[i]);
	}
	g_cmd_count += len;
}
int ProtoMan::ProtoType()
{
	return pro_type;
}


bool ProtoMan::DecodeCmdMsg(unsigned char* cmd, int cmd_len, struct CmdMsg** out_msg)
{
	if (cmd_len<8)
	{
		return false;
	}
	CmdMsg* msg = (struct CmdMsg*)malloc(sizeof(CmdMsg));
	memset(msg, 0, sizeof(CmdMsg));
	msg->stype = cmd[0] | (cmd[1] << 8);
	msg->ctype = cmd[2] | (cmd[3] << 8);
	msg->utag = cmd[4] | (cmd[5] << 8) | (cmd[6] << 16) | (cmd[7] << 24);
	if (cmd_len==8)
	{
		msg->body = NULL;
	}
	else
	{
		//此处可解密


		if (pro_type == PROTO_JSON)
		{
			int json_len = cmd_len - 8 + 1;
			char* json_str = (char*)malloc(json_len+1);
			memcpy(json_str, cmd + 8, json_len);
			json_str[json_len] = 0;
			msg->body = (void*)json_str;
		}
		else
		{
			if (g_pf_map[msg->ctype] == NULL || msg->ctype < 0 || msg->ctype >= g_cmd_count)
			{
				free(msg);
				*out_msg = NULL;
				return false;
			}
			google::protobuf::Message* pro_msg = create_message(g_pf_map[msg->ctype]);
			if (pro_msg==NULL)
			{
				free(msg);
				*out_msg = NULL;
				return false;
			}
			if (!pro_msg->ParseFromArray(cmd + 8, cmd_len - 8))
			{
				free(msg);
				*out_msg = NULL;
				release_message(pro_msg);
				return false;
			}
			msg->body = (void*)pro_msg;
		}
	}
	*out_msg = msg;
	return true;
}

void ProtoMan::CmdMsgFree(struct CmdMsg* msg)
{
	if (msg->body)
	{
		if (pro_type==PROTO_JSON)
		{
			free(msg->body);
			msg->body = NULL;
		}
		else
		{
			google::protobuf::Message* pro_msg = (google::protobuf::Message*)msg->body;
			delete pro_msg;
			msg->body = NULL;
		}
	}
	free(msg);
}

unsigned char* ProtoMan::EncodeMsgToRaw(const struct CmdMsg* msg, int* out_len)
{
	int raw_len = 0;
	unsigned char* raw_data = NULL;
	*out_len = 0;

	//此处可加密


	if (pro_type==PROTO_JSON)
	{
		char* json_str = (char*)msg->body;
		int len = strlen(json_str) + 1;
		raw_data =(unsigned char*) malloc(8 + len);
		memcpy(raw_data + 8, json_str, len -1);
		raw_data[len+8] = 0;
		*out_len = len + 8;
	}
	else
	{
		google::protobuf::Message* msg1 = (google::protobuf::Message*)msg->body;
		int pf_len = msg1->ByteSize();
		raw_data = (unsigned char*)malloc(pf_len + 8);
		if (!msg1->SerializePartialToArray(raw_data+8,pf_len))
		{
			free(raw_data);
			return NULL;
		}
		*out_len = (pf_len + 8);
		raw_data[0] = (msg->stype & 0x000000ff);
		raw_data[1] = (msg->stype & 0x0000ff00)>>8;
		raw_data[2] = (msg->stype & 0x000000ff);
		raw_data[3] = (msg->stype & 0x0000ff00)>>8;
		memcpy(raw_data + 4, &msg->utag, 4);
	}
	return raw_data;
}
void ProtoMan::MsgRawFree(unsigned char* raw)
{
	free(raw);
}
