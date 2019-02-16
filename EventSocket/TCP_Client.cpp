#include "TCP_Client.h"
#include <event2/event.h>
#include <event2/event_compat.h>
#include <event2/event_struct.h>
#include <ws2tcpip.h>

#ifdef _WIN32
#include <process.h>
#else
#include <pthread.h>
#endif

#define BUF_SIZE 2048

TCP_Client::TCP_Client()
	: m_bLoopStopping(true)
	, m_bLoopTheadHasStarted(false)	
{
	m_buffRead = new char[BUF_SIZE];
}


TCP_Client::~TCP_Client()
{
	delete[] m_buffRead;
}

int TCP_Client::setup(const char* ip, u_short port)
{
	if (isValid())
		return -99;

	int sock_res = _setupSocket(ip, port);
	if (sock_res != 0)
		return sock_res;

	int event_res = _setupEvent();
	if (event_res != 0)
	{
		closeSocket();
		return event_res;
	}

	//����socket�������̷���
	int opt = 1;
	setsockopt(getSocketFd(), IPPROTO_TCP, TCP_NODELAY, (char *)&opt, static_cast<socklen_t>(sizeof(opt)));

	onConnected();

	return 0;
}

int TCP_Client::loop()
{
	m_bLoopStopping = false;

	int res = 0;
	do
	{
		if (ev_base == NULL)
		{
			res = 1;
			break;
		}

		//ѭ��
		if (0 != event_base_dispatch(ev_base))
		{
			res = 2;
			break;
		}
	} while (0);

	//----������Դ-------------
	if (ev_listen)
	{
		event_del(ev_listen);
		free(ev_listen);
		ev_listen = NULL;
	}
	if (ev_base)
	{
		event_base_free(ev_base);
		ev_base = NULL;
	}
	closeSocket();

	onClosed();

	return 0;
}

int TCP_Client::loop_in_new_thread()
{
	m_bLoopTheadHasStarted = false;
#ifdef _WIN32
	unsigned  uiThreadID = 0;
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, TCP_Client::_process_loop_in_new_thread, (void*)this, 0, &uiThreadID);
	while (!m_bLoopTheadHasStarted)
		Sleep(1);
#else
	pthread_t thread;
	pthread_create(&thread, NULL, TCP_Client::_process_loop_in_new_thread, (void*)this);
	pthread_detach(thread);
	while (!m_bLoopTheadHasStarted)
		usleep(1);
#endif
		
	return 0;
}

int TCP_Client::_stopLoop(bool waiting)
{
	m_bLoopStopping = true;

	if (ev_base == NULL)
		return 1;

	int res = 0;

	if (waiting)
		res = event_base_loopexit(ev_base, NULL);
	else
		res = event_base_loopbreak(ev_base);

	return res;
}

int TCP_Client::send(const char* data, int datalen /*= 0*/)
{
	if (!isValid())
		return -1;

	if (datalen == 0)
		datalen = (int)strlen(data) + 1;//+1Ϊ�ַ�������\0

	int write_num =
#ifdef _WIN32
		::send(getSocketFd(), data, datalen, 0);
#else
		write(getSocketFd(), data, datalen);
#endif

	return write_num;
}

void TCP_Client::onConnected()
{

}

void TCP_Client::onRecv(const char* data, int dataLen)
{

}

void TCP_Client::onClosed()
{

}

intptr_t TCP_Client::_createSocket()
{
	intptr_t fd = socket(AF_INET, SOCK_STREAM, 0);
	return fd;
}

void TCP_Client::closeSocket()
{
	if (!isValid())
		return;

#ifdef _WIN32
	closesocket(getSocketFd());
#else
	close(getSocketFd());
#endif
	setSocketFd(-1);
}

int TCP_Client::_setupSocket(const char* ip, u_short port)
{
	setSocketFd(_createSocket());
	if (!isValid())
		return -1;

	struct sockaddr_in remote_addr; //�������������ַ�ṹ��
	memset(&remote_addr, 0, sizeof(remote_addr)); //���ݳ�ʼ��--����
	remote_addr.sin_family = AF_INET; //����ΪIPͨ��
	remote_addr.sin_addr.s_addr = inet_addr(ip);//������IP��ַ
	remote_addr.sin_port = htons(port); //�������˿ں�
	int con_result = ::connect(getSocketFd(), (struct sockaddr*) &remote_addr, sizeof(struct sockaddr));
	if (con_result < 0) 
	{
		closeSocket();
		return -2;
	}

	return 0;
}

int TCP_Client::_setupEvent()
{
	if (!isValid())
		return 1;

	//-------��ʼ��base,д�¼��Ͷ��¼�--------
	ev_base = event_base_new();
	ev_listen = (struct event*)malloc(sizeof(struct event));//�������¼��󣬴�socket��ȡ������
	//-----�Զ��¼�������Ӧ������------------
	event_set(ev_listen, getSocketFd(), EV_READ | EV_PERSIST, TCP_Client::_static_on_read, (void*)this);
	int ev_base_set_res = event_base_set(ev_base, ev_listen);
	if (0 != ev_base_set_res)
	{
		//����
		event_base_free(ev_base);
		ev_base = NULL;
		free(ev_listen);
		ev_listen = NULL;
		return 2;
	}

	int ev_add_res = event_add(ev_listen, NULL);
	if (0 != ev_add_res)
	{
		//����
		event_base_free(ev_base);
		ev_base = NULL;
		free(ev_listen);
		ev_listen = NULL;
		return 3;
	}

	return 0;
}

void TCP_Client::_static_on_read(intptr_t sock, short event, void* arg)
{
	if (NULL == arg)
		return;

	TCP_Client* _THIS = (TCP_Client*)arg;//��ȡ�������Ĳ���

	if (_THIS->m_bLoopStopping)
		return;

	//--����Ӧ����whileһֱѭ��������������libevent��ֻ�ڿ��Զ���ʱ��Ŵ���_static_on_read(),�ʲ�����while��
	_THIS->m_buffSize =
#ifdef _WIN32
		recv(sock, _THIS->m_buffRead, BUF_SIZE, 0);
#else
		read(sock, _THIS->m_buffRead, BUF_SIZE);
#endif
	if (_THIS->m_buffSize <= 0)
	{//˵��socket�رգ��˳�sessionѭ��
		//ֹͣ_process_when_accepted_in_new_threadѭ��
		_THIS->_stopLoop(false);
		return;
	}

	_THIS->onRecv(_THIS->m_buffRead, _THIS->m_buffSize);
}

#ifdef _WIN32
unsigned __stdcall
#else
void*
#endif 
TCP_Client::_process_loop_in_new_thread( void* arg )
{
	TCP_Client* _THIS = (TCP_Client*)arg;
	if (_THIS == NULL)
		return 1;

	_THIS->m_bLoopTheadHasStarted = true;

	_THIS->loop();

	return 0;
}
