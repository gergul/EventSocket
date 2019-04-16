#include "DnsCache.h"
#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <netdb.h>
#endif
#include "uv_errno.h"



DnsCache::DnsCache()
{
}


DnsCache::~DnsCache()
{
}


bool DnsCache::getCacheDomainIP(const char *host, DnsItem &item, int expireSec)
{
	std::lock_guard<std::mutex> lck(_mtx);
	auto it = _mapDns.find(host);
	if (it == _mapDns.end()) 
	{
		//没有记录
		return false;
	}
	if (it->second._create_time + expireSec < time(NULL))
	{
		//已过期
		_mapDns.erase(it);
		return false;
	}
	item = it->second;
	return true;
}

void DnsCache::setCacheDomainIP(const char *host, DnsItem &item)
{
	std::lock_guard<std::mutex> lck(_mtx);
	item._create_time = time(NULL);
	_mapDns[host] = item;
}

bool DnsCache::getSystemDomainIP(const char *host, sockaddr &item)
{
	struct addrinfo *answer = nullptr;
	//阻塞式dns解析，可能被打断
	int ret = -1;
	do 
	{
		ret = getaddrinfo(host, NULL, NULL, &answer);
	} while (ret == -1 && get_uv_error(true) == UV_EINTR);

	if (!answer) 
	{
		return false;
	}
	item = *(answer->ai_addr);
	freeaddrinfo(answer);
	return true;
}

DnsCache & DnsCache::Instance()
{
	static DnsCache instance;
	return instance;
}

bool DnsCache::getDomainIP(const char *host, struct sockaddr &addr, int expireSec /*= 60*/)
{
	DnsItem item;
	auto flag = getCacheDomainIP(host, item, expireSec);
	if (!flag) 
	{
		flag = getSystemDomainIP(host, item._addr);
		if (flag) 
		{
			setCacheDomainIP(host, item);
		}
	}
	if (flag) 
	{
		addr = item._addr;
	}
	return flag;
}