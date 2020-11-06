#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include "mysocket.h"
#include "programlist.h"


struct radio_t {
	std::string dns;
	int port;
	std::string radioName;
	int timeZone;
};

//相对于伦敦0时区时间
const radio_t radioInfo[] = { {"mobilekazn.serverroom.us",6914,"1300",0},
							  {"mobilekmrb.serverroom.us",6916,"1430",0},
							  {"mobilekazn.serverroom.us",6914,"900",0},//mobilekali.serverroom.us:9944
							  {"mobilewzrc.serverroom.us",6910,"1480",3},
							  {"mobilewkdm.serverroom.us",6912,"1380",3} };

#define  BUFLEN  16384


int requestHttp(MyNet  &net, int &sockfd, int radio) {
	if(sockfd > 0) closesocket(sockfd);
	sockfd = net.requestServer(radioInfo[radio].dns, radioInfo[radio].port);
	if (sockfd == -1) {
		std::cout << "request http error " << std::endl;
		return 0;
	}
	return 1;
}

int main(int argc, char **argv) {

	//program 0 1300
	if (argc < 3) {
		printf("0 ----- 1300\n");
		printf("1 ----- 1430\n");
		printf("2 ----- 900\n");
		printf("3 ----- 1480\n");
		printf("4 ----- 1380\n");
		return 0;
	}

	int  radio = atoi(argv[1]);
	if (radio > 5 || radio < 0) return  0;


	ProgramList  radioList(argv[2], radioInfo[radio].radioName, radioInfo[radio].timeZone);

	MyNet  net;
	int sockfd = net.requestServer(radioInfo[radio].dns, radioInfo[radio].port);
	if (sockfd < 0) return 0;

	fd_set fds;
	struct timeval timeout = { 60,0 };//等待时延60秒
	int nRet;
	char  buf[BUFLEN];
	int progressMP3 = 0;//控制台进度条	

	while (1) {
		FD_ZERO(&fds);
		FD_SET(sockfd, &fds);

		timeout.tv_sec = 60;
		timeout.tv_usec = 0;

		int errNum = select(sockfd + 1, &fds, NULL, NULL, &timeout);

		if (errNum < 0) {
			break;
		}
		else if (errNum == 0) {
			
			if (!requestHttp(net, sockfd, radio)) continue;
		}
		else {

			if (FD_ISSET(sockfd, &fds)) {
				nRet = recv(sockfd, buf, BUFLEN, 0);
				if (nRet == 0) {					
					if (!requestHttp(net, sockfd, radio)) continue;
				}
				if (nRet == -1) {
					std::cout << "recv error " << std::endl;
					continue;
				}

				progressMP3+= nRet;
				if (progressMP3 >= 16 * 1024 * 60) {
					std::cout << "#";
					fflush(stdout);
					progressMP3 = 0;
				}
				if (radioList.saveFile(buf, nRet)) {
					if (!requestHttp(net, sockfd, radio)) continue;					
				}
			}
		}
	}
	
	closesocket(sockfd);
	return 0;
}
