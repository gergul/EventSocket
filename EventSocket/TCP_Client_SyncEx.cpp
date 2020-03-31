#include "TCP_Client_SyncEx.h"
#ifndef __linux__
#include <Windows.h>
#else
#include <unistd.h>
#include <time.h>
#endif

#ifndef __linux__
unsigned long long GetSysTickCount64()
{
	static LARGE_INTEGER TicksPerSecond = { 0 };
	LARGE_INTEGER Tick;

	if (!TicksPerSecond.QuadPart)
		QueryPerformanceFrequency(&TicksPerSecond);

	QueryPerformanceCounter(&Tick);

	unsigned long long Seconds = Tick.QuadPart / TicksPerSecond.QuadPart;
	unsigned long long LeftPart = Tick.QuadPart - (TicksPerSecond.QuadPart*Seconds);
	unsigned long long MillSeconds = LeftPart * 1000 / TicksPerSecond.QuadPart;
	unsigned long long Ret = Seconds * 1000 + MillSeconds;
	_ASSERT(Ret > 0);
	return Ret;
}
#else
unsigned long long GetSysTickCount64()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
}
#endif


int GetCurThreadId()
{
#ifndef __linux__
	return GetCurrentThreadId();
#else
	return gettid();
#endif
}


TCP_Client_SyncEx::TCP_Client_SyncEx()
{
}


TCP_Client_SyncEx::~TCP_Client_SyncEx()
{
}

int TCP_Client_SyncEx::send(const char* data, int datalen /*= 0*/)
{
	unsigned long long tick = GetSysTickCount64();
	int curThreadId = GetCurThreadId();

	char strID[256];
	sprintf(strID, "%llu-%d|", tick, curThreadId);
	m_curSendWaitId = strID;
	int nIdLen = m_curSendWaitId.length();

	if (datalen == 0)
		datalen = (int)strlen(data) + 1;//+1为字符串最后的\0
	char* dataFormated = new char[datalen + nIdLen];
	memcpy(dataFormated, strID, nIdLen);
	memcpy(dataFormated + nIdLen, data, datalen);

	int nRet = TCP_Client::send(dataFormated, datalen + nIdLen);
	delete[] dataFormated;

	return nRet;
}

void TCP_Client_SyncEx::onRecv(const char* data, int dataLen)
{
	int nIdLen = m_curSendWaitId.length();
	if (dataLen >= nIdLen)
	{
		bool bStartWith = true;
		for (int i = 0; i < nIdLen; ++i)
		{
			if (data[i] != m_curSendWaitId[i])
			{
				bStartWith = false;
				break;
			}
		}

		if (bStartWith)
		{
			TCP_Client_Sync::onRecv(data + nIdLen, dataLen - nIdLen);
			return;
		}
	}

	//到这里的都是垃圾数据
#ifdef _DEBUG
	printf("垃圾数据：%s", data);
#endif
}
