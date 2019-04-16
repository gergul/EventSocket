#include "TCP_Server.h"
#include <iostream>
#include "TCP_Session_SyncEx.h"

#define TEST_SYNC 0

class MySession
	: public TCP_Session
{
public:
	MySession(intptr_t accept_fd)
	{
		m_remoteIp = Net_Helper::GetRemoteIP(accept_fd);
		m_remotePort = Net_Helper::GetRemotePort(accept_fd);
		std::cout << m_remoteIp << ":" << m_remotePort << ":new TCP_Session()" << std::endl;
	}

	virtual void onClosed() override
	{
		std::cout << m_remoteIp << ":" << m_remotePort << ":onClose()" << std::endl;
	}

	virtual void onRecv(const char* data, int dataLen) override
	{
		char* buf = new char[dataLen + 1];
		memcpy(buf, data, dataLen);
		buf[dataLen] = '\0';
		std::cout << m_remoteIp << ":" << m_remotePort << "-->" << buf << std::endl;
		delete[] buf;

		Sleep(2000);

		send(data, dataLen);
	}

private:
	std::string m_remoteIp;
	u_short m_remotePort;
};

class MySession_SyncEx
	: public TCP_Session_SyncEx
{
public:
	MySession_SyncEx()
	{

	}

	virtual void onRecvEx(const char* data, int dataLen) override
	{
		send(data, dataLen);
	}
};

class MyServer
	: public TCP_Server
{
public:
	virtual class TCP_Session* onCreateSession(intptr_t accept_fd) override
	{
		if (createSyncEx)
			return new MySession_SyncEx;
		return new MySession(accept_fd);
	}

	bool createSyncEx{ false };
};

void main()
{
	MyServer tcpServer;
#if (defined TEST_SYNC) && (TEST_SYNC == 1)
	tcpServer.createSyncEx = true;
#endif
	if (tcpServer.setup("0.0.0.0", 9000) == 0)
		tcpServer.loop_in_new_thread();
	
	while (1)
		Sleep(100000);
}