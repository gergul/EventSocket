#include "EventSocketBase.h"

EventSocketBase::EventSocketBase()
	: ev_base(0)
	, ev_listen(0)
	, m_socket_fd(-1)
	, m_buffSize(0)
{
}


EventSocketBase::~EventSocketBase()
{
}
