#include "service.h"
#include "session.h"
#include "proto_man.h"
#include "service_man.h"

#include <stdlib.h>
#include <string.h>

#define MAX_SERVICE 128
static Service* g_service_set[MAX_SERVICE];

void ServiceMan::Init()
{
	memset(g_service_set, 0, sizeof(g_service_set));
}

bool ServiceMan::RegisterService(int stype, Service* s)
{
	if (stype<0||stype>= MAX_SERVICE)
	{
		return false;
	}
	if (g_service_set[stype])
	{
		return false;
	}
	g_service_set[stype] = s;
	return true;
}

bool ServiceMan::OnRecvCmdMsg(Session* s, struct CmdMsg* msg)
{
	if (g_service_set[msg->stype]==NULL)
	{
		return false;
	}
	return g_service_set[msg->stype]->OnSessionRecvCmd(s,msg);
}

void ServiceMan::OnSessionDisconnect(Session* s)
{

	for (int i = 0; i < MAX_SERVICE; i++)
	{
		if (g_service_set[i]==NULL)
		{
			continue;
		}
		g_service_set[i]->OnSessionDisconnect(s);
	}
}