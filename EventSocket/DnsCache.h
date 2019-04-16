#pragma once
#include <corecrt.h>
#ifdef _WIN32
#include <winsock2.h>
#endif
#include <mutex>
#include <unordered_map>

class DnsCache 
{
public:

	static DnsCache &Instance();
	
	bool getDomainIP(const char *host, struct sockaddr &addr, int expireSec = 60);

private:
	DnsCache();
	~DnsCache();

	class DnsItem 
	{
	public:
		struct sockaddr _addr;
		time_t _create_time;
	};

	bool getCacheDomainIP(const char *host, DnsItem &item, int expireSec);
	void setCacheDomainIP(const char *host, DnsItem &item);
	bool getSystemDomainIP(const char *host, sockaddr &item);

private:
	std::mutex _mtx;
	std::unordered_map<std::string, DnsItem> _mapDns;
};


