#pragma once
#include "Net_Helper.h"
#include <mutex>
#include "EventSocketBase.h"

class TCP_Server
{
public:
	TCP_Server();
	virtual ~TCP_Server();

public:
	virtual int setup(const char* ip, u_short port);
	virtual int loop();
	virtual int loop_in_new_thread();
	virtual void closeSocket();

public:
	virtual class TCP_Session* onCreateSession(intptr_t accept_fd);
	virtual void onEndSession(class TCP_Session *pSession);
	virtual void onClosed();

protected:
	intptr_t _createSocket();

	int _setupSocket(const char* ip, u_short port);
	int _setupEvent();
	
	int _stopLoop(bool waiting);

protected:
	static void _static_on_accept(intptr_t sock, short event, void* arg);
	static void _create_new_accept_thread(TCP_Server* _THIS, intptr_t accept_fd);
	static 
#ifdef _WIN32
		unsigned __stdcall
#else
		void*
#endif
		_process_when_accepted_in_new_thread(void* arg);

	static
#ifdef _WIN32
		unsigned __stdcall
#else
		void*
#endif
		_process_loop_in_new_thread(void* arg);
	
protected:
	intptr_t m_socket_fd;

	struct event_base* ev_base;
	struct event* ev_listen;
};


class TCP_Session
	: public EventSocketBase
{
public:
	TCP_Session();
	virtual ~TCP_Session();

public:
	virtual int setup(intptr_t sock, class TCP_Server* server);
	virtual int loop() override;
	virtual void closeSocket() override;
	virtual int send(const char* data, int datalen = 0) override;

	void setAutoDelete(bool b);
	bool isAutoDelete();

public:
	virtual void onRecv(const char* data, int dataLen) override;
	virtual void onClosed() override;

protected:
	static void _static_on_read(intptr_t sock, short event, void* arg);

protected:
	int _stopLoop(bool waiting);

protected:
	class TCP_Server* m_server;
	bool m_bAutoRelease;

};