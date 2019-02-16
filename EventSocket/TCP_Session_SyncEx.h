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
	///获得数据的事件
	//  获得的是带标识的原始数据
	virtual void onRecv(const char* data, int dataLen) override;
	///获得数据的事件
	//  获得的是不带标识的原始数据
	virtual void onRecvEx(const char* data, int dataLen);

protected:
	std::string m_curSendWaitId;
};

