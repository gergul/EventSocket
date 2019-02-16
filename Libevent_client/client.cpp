#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string>
#include <string.h>
#include <event2/event.h>
#include <event2/event_compat.h>
#include <event2/event_struct.h>
#include <process.h>
#include <ws2tcpip.h>
#include <io.h>

#include <winsock2.h>
#pragma comment(lib,"ws2_32.lib")

using namespace std;

#define BUF_SIZE 256

/**
* ���ӵ�server�ˣ�����ɹ�������fd�����ʧ�ܷ���-1
*/
int connectServer(char* ip, int port) {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	cout << "fd= " << fd << endl;
	if (-1 == fd) {
		cout << "Error, connectServer() quit" << endl;
		return -1;
	}
	struct sockaddr_in remote_addr; //�������������ַ�ṹ��
	memset(&remote_addr, 0, sizeof(remote_addr)); //���ݳ�ʼ��--����
	remote_addr.sin_family = AF_INET; //����ΪIPͨ��
	remote_addr.sin_addr.s_addr = inet_addr(ip);//������IP��ַ
	remote_addr.sin_port = htons(port); //�������˿ں�
	int con_result = connect(fd, (struct sockaddr*) &remote_addr, sizeof(struct sockaddr));
	if (con_result < 0) {
		cout << "Connect Error!" << endl;
#ifdef _WIN32
		closesocket(fd);
#else
		close(fd);
#endif
		return -1;
	}
	cout << "con_result=" << con_result << endl;
	return fd;
}

void on_read(int sock, short event, void* arg)
{
	char* buffer = new char[BUF_SIZE];
	memset(buffer, 0, sizeof(char)*BUF_SIZE);
	//--����Ӧ����whileһֱѭ��������������libevent��ֻ�ڿ��Զ���ʱ��Ŵ���on_read(),�ʲ�����while��
	int size =
#ifdef _WIN32
	recv(sock, buffer, BUF_SIZE, 0);
#else
	read(sock, buffer, BUF_SIZE);
#endif
	if (0 == size) {//˵��socket�ر�
		cout << "read size is 0 for socket:" << sock << endl;
		struct event* read_ev = (struct event*)arg;
		if (NULL != read_ev) {
			event_del(read_ev);
			free(read_ev);
		}
#ifdef _WIN32
		closesocket(sock);
#else
		close(sock);
#endif

		return;
	}
	cout << "Received from server---" << buffer << endl;
	delete[]buffer;
}

#ifdef _WIN32
unsigned __stdcall
#else
void*
#endif
init_read_event(void* arg) {
	long long_sock = (long)arg;
	int sock = (int)long_sock;
	//-----��ʼ��libevent�����ûص�����on_read()------------
	struct event_base* base = event_base_new();
	struct event* read_ev = (struct event*)malloc(sizeof(struct event));//�������¼��󣬴�socket��ȡ������
	event_set(read_ev, sock, EV_READ | EV_PERSIST, on_read, read_ev);
	event_base_set(base, read_ev);
	event_add(read_ev, NULL);

	event_base_dispatch(base);
	//--------------
	event_base_free(base);

	return 0;
}
/**
* ����һ�����̣߳������߳����ʼ��libevent���¼���������ã�������event_base_dispatch
*/
void init_read_event_thread(int sock) {
#ifdef _WIN32
	unsigned  uiThreadID = 0;
	HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, init_read_event, (void*)sock, 0, &uiThreadID);
#else
	pthread_t thread;
	pthread_create(&thread, NULL, init_read_event, (void*)sock);
	pthread_detach(thread);
#endif
}

int main() 
{
#ifdef _WIN32
	//��ʼ��WSA  
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	if (WSAStartup(sockVersion, &wsaData) != 0)
	{
		std::cout << "WSAStartup failed." << std::endl;
		exit(0);
	}
#endif

	cout << "main started" << endl; // prints Hello World!!!
	cout << "Please input server IP:" << endl;
	char ip[16];
	cin >> ip;
	cout << "Please input port:" << endl;
	int port;
	cin >> port;
	cout << "ServerIP is " << ip << " ,port=" << port << endl;
	int socket_fd = connectServer(ip, port);
	cout << "socket_fd=" << socket_fd << endl;
	init_read_event_thread(socket_fd);
	//--------------------------
	char buffer[BUF_SIZE];
	bool isBreak = false;
	while (!isBreak) {
		cout << "Input your data to server(\'q\' or \"quit\" to exit)" << endl;
		cin >> buffer;
		if (strcmp("q", buffer) == 0 || strcmp("quit", buffer) == 0) {
			isBreak = true;
#ifdef _WIN32
			closesocket(socket_fd);
#else
			close(socket_fd);
#endif
			break;
		}
		cout << "Your input is " << buffer << endl;
		int write_num =
#ifdef _WIN32
		::send(socket_fd, buffer, strlen(buffer), 0);
#else
		write(socket_fd, buffer, strlen(buffer));
#endif
		cout << write_num << " characters written" << endl;
		Sleep(2);
	}
	cout << "main finished" << endl;

#ifdef _WIN32
	//
	WSACleanup();
#endif

	return 0;
}
