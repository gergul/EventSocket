#pragma once
#include "TCP_Client_Sync.h"

class TCP_Client_SyncEx :
	public TCP_Client_Sync
{
public:
	TCP_Client_SyncEx();
	virtual ~TCP_Client_SyncEx();

	virtual int send(const char* data, int datalen = 0) override;

protected:
	virtual void onRecv(const char* data, int dataLen) override;

protected:
	std::string m_curSendWaitId;
};

