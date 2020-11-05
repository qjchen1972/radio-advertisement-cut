#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "mysocket.h"
#include"radioer.h"


#define  BUFLEN  16384
const std::string radio_dir[] = {"1300", "beijing", "taiwan"};


int main(int argc, char **argv) {

#ifdef _WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif


	std::vector< Radioer*> radio_set;
	for (int i = 0; i < sizeof(radio_dir) / sizeof(std::string); i++) {
		Radioer *radioer = new Radioer(radio_dir[i]);
		radio_set.push_back(radioer);
	}
	//printf("size %d \n", radio_set.size());

	time_t now;
	char  buf[BUFLEN];


	while (1) {
		int fdmax = -1;
		fd_set read_fds;
		FD_ZERO(&read_fds);
		for (int i = 0; i < radio_set.size(); i++) {
			if (radio_set[i]->sockfd > 0) {
				radio_set[i]->protocol_request();
				if (radio_set[i]->sockfd > fdmax) fdmax = radio_set[i]->sockfd;
				FD_SET(radio_set[i]->sockfd, &read_fds);
			}
			else radio_set[i]->onConnect();
		}

		struct timeval timeout;
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;
		int errNum = select(fdmax + 1, &read_fds, NULL, NULL, &timeout);
		if (errNum > 0) {
			for (int i = 0; i < radio_set.size(); i++) {
				if (FD_ISSET(radio_set[i]->sockfd, &read_fds)) {
					int nRet = recv(radio_set[i]->sockfd, buf, BUFLEN, 0);
					if (nRet > 0) {
						radio_set[i]->protocol_proc(buf, nRet);
					}
					else radio_set[i]->onClose();
				}
			}
		}

		time(&now);
		for (int i = 0; i < radio_set.size(); i++)
			radio_set[i]->timeout_proc(now);
	}

#ifdef _WIN32
	WSACleanup();
#endif
	return 0;
}
