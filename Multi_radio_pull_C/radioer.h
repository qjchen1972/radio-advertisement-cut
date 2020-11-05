#pragma once
#include <iostream>
#include <string>
#include <vector>
#include<list>
#include<set>
#include <fstream>
#include <sstream>
#include <time.h>
#include "mysocket.h"
#include "download.h"

#define  BUF_LEN 255
#define  TIME_OUT 60

#define  WAIT_REQUEST 1
#define  WAIT_PARAM 2
#define  WAIT_DATA_LEN 3
#define  RECVDATA  4


class Radioer {
public:
	Radioer(std::string  dir) {
		get_config(dir);
		m_downloader = new Downloader(name, type, timezone, download_path, program_path);
	}

	~Radioer() {
		if (m_downloader) delete m_downloader;
	}

	void onConnect() {
		MyNet net;
		int fd = net.connectServer(dns, port);
		if (fd < 0) return;
		sockfd = fd;
		status = WAIT_REQUEST;
		time(&last_time);
		m_seq = -1;
	}

	void onClose() {
		if (sockfd > 0) {
			closesocket(sockfd);
			sockfd = -1;
			printf("%s close soket \n", name.c_str());
		}
	}
	
	void protocol_request(int video_type=0) {
		if (status != WAIT_REQUEST) return;
		if (wait_flag && need_wait()) return;		
		wait_flag = false;
		if(video_type)
			live_proc(type, status);
		else
			demand_proc(type, status);
	}	

	void protocol_proc(char* buf, int len) {
		if (len <= 0) return;
		time(&last_time);
		if(status == RECVDATA) {
			m_downloader->write_file(buf, len);
			if (type == 0) {
				m_cur_len += len;
				if (m_cur_len >= m_cont_len) {
					status = WAIT_REQUEST;
					m_cur_len = 0;
					m_seq++;
				}
			}
		}
		else if (status == WAIT_PARAM || status == WAIT_DATA_LEN) {
			std::string head, body;
			if (!get_resp(buf, len, head, body)) {
				status = WAIT_REQUEST;
				wait_flag = true;
				return;
			}
			if (status == WAIT_PARAM) {
				get_hls_param(body, m_seq, prefix, hls_url);
				status = WAIT_REQUEST;
			}
			else {
				m_cont_len = get_body_len(head);
				status = RECVDATA;
				if (body.length() > 0) {
					m_downloader->write_file(body.c_str(), body.length());
					m_cur_len += body.length();
				}
			}
		}
		else return;
	}

	void timeout_proc(time_t now) {
		if (now - last_time >= TIME_OUT) {
			printf("%s timeout \n", name.c_str());
			onClose();
		}
	}	
	

	int sockfd = -1;
	
private:

	std::string name;
	int8_t type;
	std::string dns;
	int16_t port;
	int timezone;
	std::string program_path;
	std::string download_path;
	std::string request_str;
	std::string m3u8_dir;

	time_t last_time = 0;
	int m_seq = -1;

	int m_cont_len = -1;
	int m_cur_len = 0;

	int status = WAIT_REQUEST;
	std::string  prefix;

	Downloader *m_downloader = nullptr;
	bool wait_flag = false;

	//Ö±²¥
	std::list<std::string> hls_url;
	std::set<std::string> downloaded_url;
	
	
	int add_downloaded_url(std::string url) {
		auto ans = downloaded_url.insert(url);
		if (!ans.second) 	return 0;		
		if (downloaded_url.size() > 30) 
			downloaded_url.erase(downloaded_url.begin());		
		return 1;
	}

	int get_hls_url(std::string &url){
		while (hls_url.size() > 0) {
			url = hls_url.front();
			hls_url.pop_front();
			if (add_downloaded_url(url)) return 1;
		}
		return 0;
	}

	void live_proc(int type, int& status) {
		std::string url;
		if (get_hls_url(url)) {
			if (!data_request(url, 1)) return;
			status = WAIT_DATA_LEN;
		}
		else {
			if (!http_request()) return;			
			if (type == 1) status = RECVDATA;
			else status = WAIT_PARAM;
		}
	}

	int need_wait() {
		time_t now;
		time(&now);
		if (now - last_time <= 10) return 1;
		return 0;
	}

	void demand_proc(int type, int& status) {
		if (m_seq < 0) {
			if (!http_request()) return;			
			if (type == 1) status = RECVDATA;
			else status = WAIT_PARAM;
		}
		else {
			if (!data_request(prefix)) return;
			status = WAIT_DATA_LEN;
		}
	}

	int get_config(std::string  dir) {

		std::ifstream of;
		std::string config = dir + "/config.txt";
		of.open(config, std::ios::in);
		if (!of.is_open()) {
			std::cout << config << " open err " << std::endl;
			return 0;
		}
		char temp[BUF_LEN];
		while (1) {
			of.getline(temp, BUF_LEN);
			if (of.fail()) return 0;
			if (temp[0] == '#') continue;
			std::stringstream ss;
			ss << temp;
			std::string key, value;
			ss >> key >> value;
			if (key == "name") name = value;
			else if (key == "type") type = atoi(value.c_str());
			else if (key == "dns") dns = value;
			else if(key == "port") port = atoi(value.c_str());
			else if (key == "timezone") timezone = atoi(value.c_str());
			else if (key == "progpath") program_path = dir + "/" + value;
			else if (key == "filepath") download_path = dir + "/" + value;
			else if (key == "request") request_str = value;
			else if (key == "m3u8_dir") m3u8_dir = value;
			else return 0;
		}
		of.close();
		return 1;
	}


	int  http_request() {
		std::string strRequest = "GET " + request_str + " HTTP/1.1\r\n";
		strRequest += "Host: " + dns + "\r\n\r\n";
		//std::cout << strRequest << std::endl;
		if (send(sockfd, strRequest.c_str(), strRequest.size(), 0) <= 0) {
			closesocket(sockfd);
			return 0;
		}		
		return 1;
	}

	int  data_request(std::string url, int type=0) {
		if (m_seq < 0) return 0;
		std::string req_url;

		if (type)
			req_url = m3u8_dir + "/" + url;
		else
			req_url = m3u8_dir + "/" + url + std::to_string(m_seq) + ".ts";
	
		std::string strRequest = "GET " + req_url + " HTTP/1.1\r\n";
		strRequest += "Host: " + dns + "\r\n\r\n";
		//std::cout<< strRequest << std::endl;
		if (send(sockfd, strRequest.c_str(), strRequest.size(), 0) <= 0) {
			closesocket(sockfd);
			return 0;
		}		
		return 1;
	}

	void trim(std::string &str)
	{
		int s = str.find_first_not_of(" ");
		int e = str.find_last_not_of(" ");
		str = str.substr(s, e - s + 1);
	}

	int get_resp(char* buf, int len, std::string& head, std::string &body) {
		std::string resp(buf, len);

		size_t pos;
		if ((pos = resp.find("\r\n\r\n")) != resp.npos) {
			head = resp.substr(0, pos);
			body = resp.substr(pos + 4);
		}
		else if ((pos = resp.find("\n\n")) != resp.npos) {
			head = resp.substr(0, pos);
			body = resp.substr(pos + 2);
		}
		else return 0;

		std::string status;
		std::string line;
		if ((pos = head.find("\r\n")) != head.npos) {
			std::string line = head.substr(0, pos);
			head = head.substr(pos + 2);
			trim(line);
			if (strncmp(line.c_str(), "HTTP/", 5) == 0) {

				size_t b1, b2;
				if ((b1 = line.find(" ")) != line.npos
					&& (b2 = line.find(" ", b1 + 1)) != line.npos)
					status = line.substr(b1 + 1, b2 - b1 - 1);
			}
		}
		if (status.length() > 1 && status[0] != '2') return 0;
		return 1;
	}

	int get_body_len(std::string head) {
		size_t pos;
		std::string line;
		std::string cont_len;

		while ((pos = head.find("\r\n")) != head.npos) {
			line = head.substr(0, pos);
			head = head.substr(pos + 2);
			trim(line);
			if (strncmp(line.c_str(), "Content-Length", 14) == 0) {
				size_t b1;
				if ((b1 = line.find(":")) != line.npos) {
					cont_len = line.substr(b1 + 1);
					trim(cont_len);
					return atoi(cont_len.c_str());
				}
			}
		}
		return -1;
	}

	void get_hls_param(std::string body, int &seq, std::string &prefix, std::list<std::string> &url) {

		size_t pos;
		std::string line;
		std::string strseq;
		bool url_flag = false;
		bool prefix_flag = false;
	
		while ((pos = body.find("\n")) != body.npos) {
			line = body.substr(0, pos);
			body = body.substr(pos + 1);
			trim(line);
			if (url_flag) {
				if (!prefix_flag) {					
					if (strseq.empty()) {
						seq = 0;
						strseq = "0";
					}
					if ((pos = line.find(strseq)) != line.npos) {
						prefix = line.substr(0, pos);
					}
					prefix_flag = true;
				}
				url.push_back(line);
				url_flag = false;
			}
			if (strncmp(line.c_str(), "#EXT-X-MEDIA-SEQUENCE", 21) == 0) {
				size_t b1;
				if ((b1 = line.find(":")) != line.npos) {
					strseq = line.substr(b1 + 1);
					trim(strseq);
					seq = atoi(strseq.c_str());
				}
				continue;
			}
			if (strncmp(line.c_str(), "#EXTINF", 7) == 0)
				url_flag = true;
		}		
	}

};