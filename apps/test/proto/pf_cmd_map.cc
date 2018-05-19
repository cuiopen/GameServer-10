
#include"../../netbus/proto_man.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



char* pf_cmd_map[] =
{
	"LoginReq",
	"LoginRes"
};

void InitPFCmdMap()
{
	ProtoMan::RegisterPFCmdMap(pf_cmd_map, sizeof(pf_cmd_map) / sizeof(char*));
}