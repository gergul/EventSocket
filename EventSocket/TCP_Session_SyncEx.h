#pragma once
#include "TCP_Server.h"

class TCP_Session_SyncEx :
	public TCP_Session
{
public:
	TCP_Session_SyncEx();
	virtual ~TCP_Session_SyncEx();

	virtual int send(const char* data, int datalen = 0) override;

protected:
	virtual void onRecv(const char* data, int dataLen) override;
	virtual void onRecvEx(const char* data, int dataLen);

protected:
	std::string m_curSendWaitId;
};

