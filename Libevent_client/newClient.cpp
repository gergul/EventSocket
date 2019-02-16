#include "TCP_Client.h"
#include <iostream>
#include "TCP_Client_Sync.h"
#include "TCP_Client_SyncEx.h"

class MyClient
	: public TCP_Client
{
public:
	virtual void onConnected() override
	{
		m_remoteIp = Net_Helper::GetRemoteIP(getSocketFd());
		m_remotePort = Net_Helper::GetRemotePort(getSocketFd());
		std::cout << m_remoteIp << ":" << m_remotePort << ":new Session()" << std::endl;
	}

	virtual void onClosed() override
	{
		std::cout << m_remoteIp << ":" << m_remotePort << ":onClose()" << std::endl;
	}

	virtual void onRecv(const char* data, int dataLen) override
	{
		std::cout << m_remoteIp << ":" << m_remotePort << "-->" << data << std::endl;
		Sleep(1000);
		send(data, dataLen);
	}

private:
	std::string m_remoteIp;
	u_short m_remotePort;
};

void main()
{
	TCP_Client_SyncEx client;
	int res = client.setup("127.0.0.1", 9000);
	if (0 == res)
	{
		client.loop_in_new_thread();
		const char* pBuf = NULL;
		int nBufLen = 0;
		int nWaitRet = 0;
		for (int i = 0; i < 100000; ++i)
		{
			client.sendAndWait(pBuf, nBufLen, nWaitRet, 0, "Gergul\n");
			if (pBuf)
				printf(pBuf);
			Sleep(500);
		}
	}
	
	Sleep(10000);
	client.closeSocket();

	while (true)
		Sleep(100000);
}