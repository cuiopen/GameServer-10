#ifndef __PROTO_MAN_H__
#define __PROTO_MAN_H__
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
	static void RegisterPFCmdMap(char** pf_map, int len);
	static int ProtoType();
	static bool DecodeCmdMsg(unsigned char* cmd, int cmd_len, struct CmdMsg** out_msg);
	static void CmdMsgFree(struct CmdMsg* msg);

	static unsigned char* EncodeMsgToRaw(const struct CmdMsg* msg, int* out_len);
	static void MsgRawFree(unsigned char* raw);
};




#endif // !__PROTO_MAN_H__
