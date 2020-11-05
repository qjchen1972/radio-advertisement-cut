#pragma once

#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/socket.h>
#ifdef EPOLL
#include <sys/epoll.h>
#endif
#define closesocket close
#else
#include <winsock2.h>
#include <windows.h>
#define socklen_t int
#endif
#include <string>
#include<cstring>

class MyNet {

public:
	MyNet(){}

	~MyNet() {}


	int connectServer(const std::string dns, unsigned short port) {

		struct hostent* hptr = gethostbyname(dns.c_str());
		if (!hptr) return -1;

		sockaddr  addr;
		getAddr(*(unsigned int*)hptr->h_addr_list[0], port, &addr);

		int  fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (fd < 0) return fd;
		int nRecvBuf = 64 * 1024;//ÉèÖÃÎª32K
		if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int)) != 0)
			return -1;

		if (connect(fd, (const sockaddr*)&addr, sizeof(addr)) == 0) return fd;

		closesocket(fd);
		return -1;
	}

private:		
	void getAddr(unsigned int ip, unsigned short port, sockaddr* addr) {
		memset(addr, 0, sizeof(sockaddr));
		((sockaddr_in*)addr)->sin_family = AF_INET;
#ifdef _WIN32
		((sockaddr_in*)addr)->sin_addr.S_un.S_addr = ip;
#else
		((sockaddr_in*)addr)->sin_addr.s_addr = ip;
#endif
		((sockaddr_in*)addr)->sin_port = htons(port);
	}
};