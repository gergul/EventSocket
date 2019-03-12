#pragma once
#include "Net_Helper.h"
#include "EventSocketBase.h"

class TCP_Client
	: public EventSocketBase
{
public:
	TCP_Client();
	virtual ~TCP_Client();
	
public:
	virtual int setup(const char* ip, u_short port);
	virtual int loop() override;
	virtual int loop_in_new_thread();	
	virtual void closeSocket() override;

	virtual int send(const char* data, int datalen = 0) override;
	
public:
	virtual void onConnected();
	virtual void onRecv(const char* data, int dataLen) override;
	virtual void onClosed() override;

protected:
	static void _static_on_read(intptr_t sock, short event, void* arg);
	static
#ifdef _WIN32
		unsigned __stdcall
#else
		void*
#endif
		_process_loop_in_new_thread(void* arg);
	bool m_bLoopTheadHasStarted;
	bool m_bLoopStopping;

protected:
	intptr_t _createSocket();
	int _setupSocket(const char* ip, u_short port);
	int _setupEvent();

	int _stopLoop(bool waiting);
};

