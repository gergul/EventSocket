#pragma once

#ifdef _WIN64
typedef __int64          intptr_t;
#else
typedef int              intptr_t;
#endif

class EventSocketBase
{
public:
	EventSocketBase();
	virtual ~EventSocketBase();

	static int BUF_MAX_SIZE;

public:
	void setSocketFd(intptr_t socketFd) { m_socket_fd = socketFd; }
	intptr_t getSocketFd() { return m_socket_fd; }
	bool isValid() { return m_socket_fd > -1; }
	
public:
	virtual int loop() = 0;
	virtual void closeSocket() = 0;
	virtual int send(const char* data, int datalen = 0) = 0;

protected:
	virtual void onRecv(const char* data, int dataLen) = 0;
	virtual void onClosed() = 0;

private:
	intptr_t m_socket_fd;

protected:
	struct event_base* ev_base;
	struct event* ev_listen;

	char* m_buffRead;
	int m_buffSize;
};

