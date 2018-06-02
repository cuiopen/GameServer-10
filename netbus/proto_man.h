#ifndef __PROTO_MAN_H__
#define __PROTO_MAN_H__

#include <google\protobuf\message.h>
#include <string>
#include <map>
enum {
	PROTO_JSON = 0,
	PROTO_BUF = 1,
};

struct CmdMsg {
	int stype;//服务号 2byte
	int ctype;//命令号 2byte
	unsigned int utag;//用户标识  4byte
	void* body; // JSON str 或者是message;
};

class ProtoMan {
public:
	static void Init(int proto_type);
	static void RegisterPFCmdMap(std::map<int, std::string>& map);
	static int ProtoType();
	static const char* CtypeToName(int ctype);
	static bool DecodeCmdMsg(unsigned char* cmd, int cmd_len, struct CmdMsg** out_msg);
	static void CmdMsgFree(struct CmdMsg* msg);
	static google::protobuf::Message* create_message(const char* typeName);
	static void release_message(google::protobuf::Message* msg);
	static unsigned char* EncodeMsgToRaw(const struct CmdMsg* msg, int* out_len);
	static void MsgRawFree(unsigned char* raw);
};




#endif // !__PROTO_MAN_H__
