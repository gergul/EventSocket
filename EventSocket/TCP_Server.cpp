#include "TCP_Server.h"
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

TCP_Server::TCP_Server()
	: ev_base(NULL)
	, ev_listen(NULL)
	, m_socket_fd(-1)
{
}


TCP_Server::~TCP_Server()
{
}


int TCP_Server::setup(const char* ip, u_short port)
{
	int socket_result = _setupSocket(ip, port);
	if (socket_result != 0)
		return socket_result;

	int event_result = _setupEvent();
	if (event_result != 0)
	{
		closeSocket();
		return event_result;
	}

	return 0;
}

int TCP_Server::loop()
{
	int res = 0;
	do
	{
		if (ev_base == NULL)
		{
			res = 1;
			break;
		}

		//循环
		if (0 != event_base_dispatch(ev_base))
		{
			res = 2;
			break;
		}
	} while (0);
	
	//----销毁资源-------------
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

int TCP_Server::loop_in_new_thread()
{
#ifdef _WIN32
	unsigned  uiThreadID = 0;
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, TCP_Server::_process_loop_in_new_thread, (void*)this, 0, &uiThreadID);
#else
	pthread_t thread;
	pthread_create(&thread, NULL, TCP_Server::_process_loop_in_new_thread, (void*)this);
	pthread_detach(thread);
#endif

	return 0;
}

int TCP_Server::_stopLoop(bool waiting)
{
	if (ev_base == NULL)
		return 1;

	int res = 0;

	if (waiting)
		res = event_base_loopexit(ev_base, NULL);
	else
		res = event_base_loopbreak(ev_base);

	return res;
}

void TCP_Server::closeSocket()
{
	if (m_socket_fd < 0)
		return;

#ifdef _WIN32
	closesocket(m_socket_fd);
#else
	close(m_socket_fd);
#endif
	m_socket_fd = -1;
}

class TCP_Session* TCP_Server::onCreateSession(intptr_t accept_fd)
{
	return new TCP_Session;
}

void TCP_Server::onEndSession(class TCP_Session *pSession)
{

}

void TCP_Server::onClosed()
{

}

intptr_t TCP_Server::_createSocket()
{
	intptr_t fd = socket(AF_INET, SOCK_STREAM, 0);
	return fd;
}

int TCP_Server::_setupSocket(const char* ip, u_short port)
{
	m_socket_fd = _createSocket();
	if (m_socket_fd < 0)
		return -1;

	//----为服务器主线程绑定ip和port------------------------------
	struct sockaddr_in local_addr; //服务器端网络地址结构体
	memset(&local_addr, 0, sizeof(local_addr)); //数据初始化--清零
	local_addr.sin_family = AF_INET; //设置为IP通信
	local_addr.sin_addr.s_addr = inet_addr(ip);//服务器IP地址
	local_addr.sin_port = htons(port); //服务器端口号
	int bind_result = bind(m_socket_fd, (struct sockaddr*) &local_addr, sizeof(struct sockaddr));
	if (bind_result < 0)
	{
		closeSocket();
		return -2;
	}
	//listen
	int listen_result = listen(m_socket_fd, 256);
	if (listen_result < 0)
	{
		closeSocket();
		return -3;
	}

	return 0;
}

int TCP_Server::_setupEvent()
{
	if (m_socket_fd < 0)
		return 1;

	//-----设置libevent事件，每当socket出现可读事件，就调用on_accept()------------
	ev_base = event_base_new();
	ev_listen = (struct event*)malloc(sizeof(struct event));
	event_set(ev_listen, m_socket_fd, EV_READ | EV_PERSIST, TCP_Server::_static_on_accept, (void*)this);
	int base_set_res = event_base_set(ev_base, ev_listen);
	if (base_set_res != 0)
	{
		event_base_free(ev_base);
		ev_base = NULL;
		free(ev_listen);
		ev_listen = NULL;
		return 2;
	}

	int event_add_res = event_add(ev_listen, NULL);
	if (event_add_res != 0)
	{
		event_base_free(ev_base);
		ev_base = NULL;
		free(ev_listen);
		ev_listen = NULL;
		return 3;
	}

	return 0;
}

void TCP_Server::_static_on_accept(intptr_t sock, short event, void* arg)
{
	TCP_Server* _THIS = (TCP_Server*)arg;
	if (NULL == _THIS)
		return;

	struct sockaddr_in remote_addr;
	int sin_size = sizeof(struct sockaddr_in);
	intptr_t accept_fd = accept(sock, (struct sockaddr*) &remote_addr, (socklen_t*)&sin_size);
	if (accept_fd < 0)
		return;

	TCP_Server::_create_new_accept_thread(_THIS, accept_fd);
}

struct _ACCEPT_INFO
{
	TCP_Server* _THIS;
	intptr_t accept_fd;
};
void TCP_Server::_create_new_accept_thread(TCP_Server* _THIS, intptr_t accept_fd)
{
	struct _ACCEPT_INFO* accept_info = new _ACCEPT_INFO;
	accept_info->_THIS = _THIS;
	accept_info->accept_fd = accept_fd;

#ifdef _WIN32
	unsigned  uiThreadID = 0;
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, _process_when_accepted_in_new_thread, (void*)accept_info, 0, &uiThreadID);
#else
	pthread_t thread;
	pthread_create(&thread, NULL, _process_when_accepted_in_new_thread, (void*)accept_info);
	pthread_detach(thread);
#endif
}

#ifdef _WIN32
unsigned __stdcall
#else
void*
#endif
TCP_Server::_process_when_accepted_in_new_thread(void* arg)
{
	struct _ACCEPT_INFO* accept_info = (struct _ACCEPT_INFO*)arg;
	if (accept_info == NULL)
		return 0;

	intptr_t accept_sock_fd = (intptr_t)accept_info->accept_fd;
	TCP_Server* _THIS = accept_info->_THIS;

	delete accept_info;

	if (accept_sock_fd < 0 || NULL == _THIS)
		return 0;

	//process session
	TCP_Session *pNewSession = _THIS->onCreateSession(accept_sock_fd);
	if (!pNewSession)
		return 0;

	if (0 == pNewSession->setup(accept_sock_fd, _THIS))
		pNewSession->loop();
	
	_THIS->onEndSession(pNewSession);

#ifdef _WIN32
		closesocket(accept_sock_fd);
#else
		close(accept_sock_fd);
#endif

	if (pNewSession->isAutoDelete())
		delete pNewSession;

	return 0;
}

#ifdef _WIN32
unsigned __stdcall
#else
void*
#endif 
TCP_Server::_process_loop_in_new_thread(void* arg)
{
	TCP_Server* _THIS = (TCP_Server*)arg;
	if (_THIS == NULL)
		return 1;
	
	_THIS->loop();

	return 0;
}

TCP_Session::TCP_Session()
	: m_server(NULL)
	, m_bAutoRelease(true)
{
	m_buffRead = new char[BUF_SIZE];
}

TCP_Session::~TCP_Session()
{
	_stopLoop(false);
	delete[] m_buffRead;
}

int TCP_Session::setup(intptr_t sock, class TCP_Server* server)
{
	if (sock < 0 || NULL == server)
		return 1;

	setSocketFd(sock);
	m_server = server;

	//-------初始化base,写事件和读事件--------
	ev_base = event_base_new();
	ev_listen = (struct event*)malloc(sizeof(struct event));//发生读事件后，从socket中取出数据
	//-----对读事件进行相应的设置------------
	event_set(ev_listen, getSocketFd(), EV_READ | EV_PERSIST, TCP_Session::_static_on_read, (void*)this);
	int ev_base_set_res = event_base_set(ev_base, ev_listen);
	if (0 != ev_base_set_res)
	{
		//销毁
		event_base_free(ev_base);
		ev_base = NULL;
		free(ev_listen);
		ev_listen = NULL;
		return 2;
	}

	int ev_add_res = event_add(ev_listen, NULL);
	if (0 != ev_add_res)
	{
		//销毁
		event_base_free(ev_base);
		ev_base = NULL;
		free(ev_listen);
		ev_listen = NULL;
		return 3;
	}

	int opt = 1;
	setsockopt(getSocketFd(), IPPROTO_TCP, TCP_NODELAY, (char *)&opt, static_cast<socklen_t>(sizeof(opt)));

	return 0;
}

int TCP_Session::loop()
{
	int res = 0;
	do 
	{
		if (ev_base == NULL)
		{
			res = 1;
			break;
		}

		//循环
		if (0 != event_base_dispatch(ev_base))
		{
			res = 2;
			break;
		}
	} while (0);
		
	//----销毁资源-------------
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

	return res;
}

void TCP_Session::closeSocket()
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

int TCP_Session::_stopLoop(bool waiting)
{
	if (ev_base == NULL)
		return 1;

	int res = 0;
	if (waiting)
		res = event_base_loopexit(ev_base, NULL);
	else
		res = event_base_loopbreak(ev_base);

	return res;
}


void TCP_Session::setAutoDelete(bool b)
{
	m_bAutoRelease = b;
}

bool TCP_Session::isAutoDelete()
{
	return m_bAutoRelease;
}

int TCP_Session::send(const char* data, int datalen /*= 0*/)
{
	if (!isValid())
		return -1;

	if (datalen == 0)
		datalen = (int)strlen(data) + 1;//+1为字符串最后的\0

	int write_num =
#ifdef _WIN32
		::send(getSocketFd(), data, datalen, 0);
#else
		write(getSocketFd(), data, datalen);
#endif

	return write_num;
}

void TCP_Session::onRecv(const char* data, int dataLen)
{

}

void TCP_Session::onClosed()
{

}

void TCP_Session::_static_on_read(intptr_t sock, short event, void* arg)
{
	if (NULL == arg)
		return;

	TCP_Session* _THIS = (TCP_Session*)arg;//获取传进来的参数
	
	//--本来应该用while一直循环，但由于用了libevent，只在可以读的时候才触发_static_on_read(),故不必用while了
	_THIS->m_buffSize =
#ifdef _WIN32
		recv(sock, _THIS->m_buffRead, BUF_SIZE, 0);
#else
		read(sock, _THIS->m_buffRead, BUF_SIZE);
#endif
	if (_THIS->m_buffSize <= 0)
	{//说明socket关闭，退出session循环
		//停止_process_when_accepted_in_new_thread循环
		_THIS->_stopLoop(false);
		return;
	}

	_THIS->onRecv(_THIS->m_buffRead, _THIS->m_buffSize);
}