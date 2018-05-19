#include "service.h"
#include "session.h"
#include "proto_man.h"

#include <stdlib.h>
#include <string.h>



bool Service::OnSessionRecvCmd(Session* s, CmdMsg* msg)
{
	return false;
}
void Service::OnSessionDisconnect(Session* s)
{

}