#pragma once
#include "TCP_Client.h"
#include "GLEvent.h"

class TCP_Client_Sync :
	public TCP_Client
{
public:
	TCP_Client_Sync();
	virtual ~TCP_Client_Sync();
	
	///发送数据，并等待返回
	//  dataRet - 返回的数据
	//  datalenRet - 返回的数据的长度
	//  waitRet - 等待的结果。0:正常；1:超时；-1:出错
	//  timeout - 超时时间，毫秒。为0时无限等待
	//  data - 要发送的数据
	//  datalen - 要发送的数据长度
	virtual int sendAndWait(const char*& dataRet, int& datalenRet, int& waitRet, long timeout, const char* data, int datalen = 0);

protected:
	virtual void onRecv(const char* data, int dataLen) override;

protected:
	char* m_retBuf;
	int m_bufLen;
	int m_retLen;
	event_handle m_recvEvent;
};

