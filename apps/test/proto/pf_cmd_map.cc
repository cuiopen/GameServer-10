
#include"../../netbus/proto_man.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <map>


std::map<int, std::string> map =
{
	{0,"LoginReq"},
	{ 1,"LoginRes" },
};

void InitPFCmdMap()
{
	ProtoMan::RegisterPFCmdMap(map);
}