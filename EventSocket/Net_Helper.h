#pragma once
#include <string>
#ifdef _WIN32
#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")
#endif

class Net_Helper
{
public:
	static std::string GetRemoteIP(intptr_t fd);
	static u_short GetRemotePort(intptr_t fd);
	static std::string GetLocalIP(intptr_t fd);
	static u_short GetLocalPort(intptr_t fd);
};

class WSA_STARTUP
{
public:
	WSA_STARTUP();
	~WSA_STARTUP();
};
