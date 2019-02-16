#pragma once
#include "TCP_Client.h"
#include "GLEvent.h"

class TCP_Client_Sync :
	public TCP_Client
{
public:
	TCP_Client_Sync();
	virtual ~TCP_Client_Sync();
	
	virtual int sendAndWait(const char*& dataRet, int& datalenRet, int& waitRet, long timeout, const char* data, int datalen = 0);

protected:
	virtual void onRecv(const char* data, int dataLen) override;

protected:
	char* m_retBuf;
	int m_bufLen;
	int m_retLen;
	event_handle m_recvEvent;
};

