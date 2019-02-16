#include "TCP_Client_Sync.h"


TCP_Client_Sync::TCP_Client_Sync()
	: m_retBuf(NULL)
	, m_recvEvent(NULL)
	, m_bufLen(0)
	, m_retLen(0)
{
}


TCP_Client_Sync::~TCP_Client_Sync()
{
	if (m_retBuf)
		delete[] m_retBuf;
}

int TCP_Client_Sync::sendAndWait(const char*& dataRet, int& datalenRet, int& waitRet, long timeout, const char* data, int datalen /*= 0*/)
{
	if (!m_recvEvent)
		m_recvEvent = event_create(true, true);
	event_reset(m_recvEvent);

	int nSended = send(data, datalen);
	
	if (timeout > 0)
		waitRet = event_timedwait(m_recvEvent, timeout);
	else
		waitRet = event_wait(m_recvEvent);
	
	if (waitRet == 0)
	{
		dataRet = m_retBuf;
		datalenRet = m_retLen;
	}
	else
	{
		dataRet = NULL;
		datalenRet = 0;
	}

	return nSended;
}

void TCP_Client_Sync::onRecv(const char* data, int dataLen)
{
	if (m_bufLen < dataLen)
	{
		if (m_retBuf)
			delete[] m_retBuf;
		m_retBuf = new char[dataLen];
		m_bufLen = dataLen;
	}
	
	memcpy(m_retBuf, data, dataLen);
	m_retLen = dataLen;

	event_set(m_recvEvent);
}
