#ifndef __PROTO_MAN_H__
#define __PROTO_MAN_H__
enum {
	PROTO_JSON = 0,
	PROTO_BUF = 1,
};

struct CmdMsg {
	int stype;//����� 2byte
	int ctype;//����� 2byte
	unsigned int utag;//�û���ʶ  4byte
	void* body; // JSON str ������message;
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
