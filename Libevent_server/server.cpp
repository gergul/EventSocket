#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <event2/event.h>
#include <event2/event_compat.h>
#include <event2/event_struct.h>
#include <process.h>
#include <ws2tcpip.h>
#include <io.h>

#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")

using namespace std;

#define SERVER_IP "0.0.0.0"
#define SERVER_PORT 9090
#define BUF_SIZE 4096

struct sock_ev_write {//用户写事件完成后的销毁，在on_write()中执行
	struct event* write_ev;
	char* buffer;
};
struct sock_ev {//用于读事件终止（socket断开）后的销毁
	struct event_base* base;//因为socket断掉后，读事件的loop要终止，所以要有base指针
	struct event* read_ev;
};

/**
* 销毁写事件用到的结构体
*/
void destroy_sock_ev_write(struct sock_ev_write* sock_ev_write_struct) {
	if (NULL != sock_ev_write_struct) {
		//event_del(sock_ev_write_struct->write_ev);//因为写事件没用EV_PERSIST，故不用event_del
		if (NULL != sock_ev_write_struct->write_ev) {
			free(sock_ev_write_struct->write_ev);
		}
		if (NULL != sock_ev_write_struct->buffer) {
			delete[]sock_ev_write_struct->buffer;
		}
		free(sock_ev_write_struct);
	}
}


/**
* 读事件结束后，用于销毁相应的资源
*/
void destroy_sock_ev(struct sock_ev* sock_ev_struct) {
	if (NULL == sock_ev_struct) {
		return;
	}
	event_del(sock_ev_struct->read_ev);
	event_base_loopexit(sock_ev_struct->base, NULL);//停止loop循环
	if (NULL != sock_ev_struct->read_ev) {
		free(sock_ev_struct->read_ev);
	}
	event_base_free(sock_ev_struct->base);
	//    destroy_sock_ev_write(sock_ev_struct->sock_ev_write_struct);
	free(sock_ev_struct);
}
int getSocket() {
#ifdef _WIN32
	//初始化WSA  
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0)
		return -1;
#endif

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == fd) {
		cout << "Error, fd is -1" << endl;
	}
	return fd;
}

void on_write(int sock, short event, void* arg)
{
	cout << "on_write() called, sock=" << sock << endl;
	if (NULL == arg) {
		cout << "Error! void* arg is NULL in on_write()" << endl;
		return;
	}
	struct sock_ev_write* sock_ev_write_struct = (struct sock_ev_write*)arg;

	char buffer[BUF_SIZE];
	sprintf(buffer, "fd=%d, received[%s]", sock, sock_ev_write_struct->buffer);
	//    int write_num0 = write(sock, sock_ev_write_struct->buffer, strlen(sock_ev_write_struct->buffer));
	//    int write_num = write(sock, sock_ev_write_struct->buffer, strlen(sock_ev_write_struct->buffer));
	int write_num = 
#ifdef _WIN32
		send(sock, buffer, strlen(buffer), 0);
#else
		write(sock, buffer, strlen(buffer));
#endif
	destroy_sock_ev_write(sock_ev_write_struct);
	cout << "on_write() finished, sock=" << sock << endl;
}

void on_read(int sock, short event, void* arg)
{
	cout << "on_read() called, sock=" << sock << endl;
	if (NULL == arg) {
		return;
	}
	struct sock_ev* event_struct = (struct sock_ev*) arg;//获取传进来的参数
	char* buffer = new char[BUF_SIZE];
	memset(buffer, 0, sizeof(char)*BUF_SIZE);
	//--本来应该用while一直循环，但由于用了libevent，只在可以读的时候才触发on_read(),故不必用while了
	int size = 
#ifdef _WIN32
		recv(sock, buffer, BUF_SIZE, 0);
#else
		read(sock, buffer, BUF_SIZE);
#endif
	if (0 == size) {//说明socket关闭
		cout << "read size is 0 for socket:" << sock << endl;
		destroy_sock_ev(event_struct);
#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif
		return;
	}
	cout << "on_read() finished, sock=" << sock << endl;

	//struct sock_ev_write* sock_ev_write_struct = (struct sock_ev_write*)malloc(sizeof(struct sock_ev_write));
	//sock_ev_write_struct->buffer = buffer;
	//struct event* write_ev = (struct event*)malloc(sizeof(struct event));//发生写事件（也就是只要socket缓冲区可写）时，就将反馈数据通过socket写回客户端
	//sock_ev_write_struct->write_ev = write_ev;
	//event_set(write_ev, sock, EV_WRITE, on_write, sock_ev_write_struct);
	//event_base_set(event_struct->base, write_ev);
	//event_add(write_ev, NULL);

	char bufferSend[BUF_SIZE];
	sprintf(bufferSend, "fd=%d, received.", sock);
	//    int write_num0 = write(sock, sock_ev_write_struct->buffer, strlen(sock_ev_write_struct->buffer));
	//    int write_num = write(sock, sock_ev_write_struct->buffer, strlen(sock_ev_write_struct->buffer));
	int write_num =
#ifdef _WIN32
		send(sock, bufferSend, strlen(bufferSend), 0);
#else
		write(sock, bufferSend, strlen(bufferSend));
#endif

}


/**
* main执行accept()得到新socket_fd的时候，执行这个方法
* 创建一个新线程，在新线程里反馈给client收到的信息
*/
unsigned __stdcall _process_in_new_thread_when_accepted(void* arg)
{
	long long_fd = (long)arg;
	int fd = (int)long_fd;
	if (fd < 0) {
		cout << "process_in_new_thread_when_accepted() quit!" << endl;
		return 0;
	}
	//-------初始化base,写事件和读事件--------
	struct event_base* base = event_base_new();
	struct event* read_ev = (struct event*)malloc(sizeof(struct event));//发生读事件后，从socket中取出数据

	//-------将base，read_ev,write_ev封装到一个event_struct对象里，便于销毁---------
	struct sock_ev* event_struct = (struct sock_ev*)malloc(sizeof(struct sock_ev));
	event_struct->base = base;
	event_struct->read_ev = read_ev;
	//-----对读事件进行相应的设置------------
	event_set(read_ev, fd, EV_READ | EV_PERSIST, on_read, event_struct);
	event_base_set(base, read_ev);
	event_add(read_ev, NULL);
	//--------开始libevent的loop循环-----------
	event_base_dispatch(base);

	cout << "event_base_dispatch() stopped for sock(" << fd << ")" << " in process_in_new_thread_when_accepted()" << endl;
	return 0;
}

/**
* 每当accept出一个新的socket_fd时，调用这个方法。
* 创建一个新线程，在新线程里与client做交互
*/
void accept_new_thread(int sock) {
	unsigned  uiThreadID = 0;
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, _process_in_new_thread_when_accepted, (void*)sock, 0, &uiThreadID);
}

/**
* 每当有新连接连到server时，就通过libevent调用此函数。
*    每个连接对应一个新线程
*/
void on_accept(int sock, short event, void* arg)
{
	struct sockaddr_in remote_addr;
	int sin_size = sizeof(struct sockaddr_in);
	int new_fd = accept(sock, (struct sockaddr*) &remote_addr, (socklen_t*)&sin_size);
	if (new_fd < 0) {
		cout << "Accept error in on_accept()" << endl;
		return;
	}
	cout << "new_fd accepted is " << new_fd << endl;
	accept_new_thread(new_fd);
	cout << "on_accept() finished for fd=" << new_fd << endl;
}

int main() 
{
	int fd = getSocket();
	if (fd<0) {
		cout << "Error in main(), fd<0" << endl;
	}
	cout << "main() fd=" << fd << endl;
	//----为服务器主线程绑定ip和port------------------------------
	struct sockaddr_in local_addr; //服务器端网络地址结构体
	memset(&local_addr, 0, sizeof(local_addr)); //数据初始化--清零
	local_addr.sin_family = AF_INET; //设置为IP通信
	local_addr.sin_addr.s_addr = inet_addr(SERVER_IP);//服务器IP地址
	local_addr.sin_port = htons(SERVER_PORT); //服务器端口号
	int bind_result = bind(fd, (struct sockaddr*) &local_addr, sizeof(struct sockaddr));
	if (bind_result < 0) {
		cout << "Bind Error in main()" << endl;
		return -1;
	}
	cout << "bind_result=" << bind_result << endl;
	listen(fd, 256);
	//-----设置libevent事件，每当socket出现可读事件，就调用on_accept()------------
	struct event_base* base = event_base_new();
	struct event listen_ev;
	event_set(&listen_ev, fd, EV_READ | EV_PERSIST, on_accept, NULL);
	event_base_set(base, &listen_ev);
	event_add(&listen_ev, NULL);
	//循环
	event_base_dispatch(base);

	//------以下语句理论上是不会走到的---------------------------
	cout << "event_base_dispatch() in main() finished" << endl;
	//----销毁资源-------------
	event_del(&listen_ev);
	event_base_free(base);
#ifdef _WIN32
	closesocket(fd);
#else
	close(fd);
#endif
	cout << "main() finished" << endl;

	//////////////////Windows///////////////////	
	WSACleanup();
	///////////////////////////////////////////
}