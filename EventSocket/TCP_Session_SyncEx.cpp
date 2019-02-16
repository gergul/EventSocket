#include "TCP_Session_SyncEx.h"
#include <algorithm>



TCP_Session_SyncEx::TCP_Session_SyncEx()
{
}

TCP_Session_SyncEx::~TCP_Session_SyncEx()
{
}

int TCP_Session_SyncEx::send(const char* data, int datalen /*= 0*/)
{
	if (m_curSendWaitId.empty())
		return -1;

	int nIdLen = (int)m_curSendWaitId.length();

	if (datalen == 0)
		datalen = (int)strlen(data) + 1;//+1为字符串最后的\0
	char* dataFormated = new char[datalen + nIdLen];
	memcpy(dataFormated, m_curSendWaitId.data(), nIdLen);
	memcpy(dataFormated + nIdLen, data, datalen);

	int nRet = TCP_Session::send(dataFormated, datalen + nIdLen);
	delete[] dataFormated;

	return nRet;
}

void TCP_Session_SyncEx::onRecv(const char* data, int dataLen)
{
	//wait id 部分最长的长度为256
	int nIdFind = std::min<int>(256, dataLen);
	char szId[256];
	int i = 0;
	for (; i < nIdFind; ++i)
	{
		szId[i] = data[i];

		if (data[i] == '|')
		{
			szId[i + 1] = '\0';
			break;
		}
	}
	if (i < nIdFind)
	{//带有标识的数据，有效数据
		m_curSendWaitId = szId;
		int nIdLen = (int)m_curSendWaitId.length();
		onRecvEx(data + nIdLen, dataLen - nIdLen);
		return;
	}

	//到这里为无效数据
#ifdef _DEBUG
	printf("无效数据：%s", data);
#endif
}

void TCP_Session_SyncEx::onRecvEx(const char* data, int dataLen)
{
	
}

