#include "Net_Helper.h"
#include <iostream>
#ifdef _WIN32
#include <ws2tcpip.h>
#endif

WSA_STARTUP::WSA_STARTUP()
{
#ifdef _WIN32
	//初始化WSA  
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		std::cout << "WSAStartup failed." << std::endl;
		exit(0);
	}
#endif
}

WSA_STARTUP::~WSA_STARTUP()
{
#ifdef _WIN32
	//
	WSACleanup();
#endif
}
WSA_STARTUP __wsa_startup;

std::string Net_Helper::GetRemoteIP(intptr_t fd)
{
	struct sockaddr addr;
	struct sockaddr_in* addr_v4;
	socklen_t addr_len = sizeof(addr);
	//获取remote ip and port
	if (0 == getpeername(fd, &addr, &addr_len))
	{
		if (addr.sa_family == AF_INET)
		{
			addr_v4 = (sockaddr_in*)&addr;
			return inet_ntoa(addr_v4->sin_addr);
		}
	}

	return "0.0.0.0";
}

u_short Net_Helper::GetRemotePort(intptr_t fd)
{
	struct sockaddr addr;
	struct sockaddr_in* addr_v4;
	socklen_t addr_len = sizeof(addr);
	//获取remote ip and port
	if (0 == getpeername(fd, &addr, &addr_len))
	{
		if (addr.sa_family == AF_INET)
		{
			addr_v4 = (sockaddr_in*)&addr;
			return ntohs(addr_v4->sin_port);
		}
	}

	return 0;
}

std::string Net_Helper::GetLocalIP(intptr_t fd)
{
	struct sockaddr addr;
	struct sockaddr_in* addr_v4;
	socklen_t addr_len = sizeof(addr);
	//获取local ip and port
	memset(&addr, 0, sizeof(addr));
	if (0 == getsockname(fd, &addr, &addr_len)) 
	{
		if (addr.sa_family == AF_INET)
		{
			addr_v4 = (sockaddr_in*)&addr;
			return inet_ntoa(addr_v4->sin_addr);
		}
	}

	return "0.0.0.0";
}

u_short Net_Helper::GetLocalPort(intptr_t fd)
{
	struct sockaddr addr;
	struct sockaddr_in* addr_v4;
	socklen_t addr_len = sizeof(addr);
	//获取remote ip and port
	if (0 == getsockname(fd, &addr, &addr_len))
	{
		if (addr.sa_family == AF_INET)
		{
			addr_v4 = (sockaddr_in*)&addr;
			return ntohs(addr_v4->sin_port);
		}
	}

	return 0;
}
