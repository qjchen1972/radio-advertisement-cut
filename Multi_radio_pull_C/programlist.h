#pragma once
#include <string>
#include <vector>
#include <time.h>
#include <iostream>
#include <fstream>


#pragma  pack (push,1)
/*��Ŀ��*/
struct prolist_t
{
	std::string name;//��Ŀ��
	std::string pTime;//��Ŀ��ʼ����ʱ�䣬����06:00-07:00	
	int proStatus;//�Ƿ�Ϊ�����ݵĽ�Ŀ
	std::string  host;//������
	int startTime;
};
#pragma pack(pop)


class ProgramList {

public:
	ProgramList(std::string dir) {
		m_list = weekprog_to_list(dir);
	}
	~ProgramList() {}

	int get_now_prog(int timezone, int& week, int& id, prolist_t& prog) {
		struct tm *pt = getNowTime(timezone);
		week = pt->tm_wday;
		int sec = pt->tm_hour * 3600 + pt->tm_min * 60 + pt->tm_sec;
		for (int i = m_list[week].size() - 1; i >= 0; i--)
			if (m_list[week][i].startTime <= sec) {
				id = i;
				prog = m_list[week][i];
				return 1;
			}
		return 0;
	}

private:
	std::vector<std::vector<prolist_t>> m_list;
	const std::string prog_file[7] = { "Sun.txt","Mon.txt","Tues.txt","Wed.txt","Thur.txt","Fri.txt","Sat.txt" };

	std::vector<prolist_t> file_to_list(std::string file) {

		std::vector<prolist_t> list;
		std::ifstream of;
		of.open(file, std::ios::in);
		if (!of.is_open()) {
			std::cout << file << " open err " << std::endl;
			return list;
		}
		int  temp1, temp2;
		while (!of.eof()) {
			prolist_t node;
			of >> node.name >> node.pTime >> node.proStatus >> temp1 >> temp2 >> node.host;
			if (of.fail()) break;
			int key, value;
			if (sscanf(node.pTime.substr(0, 5).c_str(), "%d:%d", &key, &value) != 2) {
				of.close();
				return list;
			}
			node.startTime = key * 3600 + value * 60;
			list.push_back(node);
		}
		of.close();
		return list;
	}

	std::vector<std::vector<prolist_t>> weekprog_to_list(std::string dir) {

		std::vector<std::vector<prolist_t>> list;
		for (int i = 0; i < sizeof(prog_file) / sizeof(std::string); i++) {
			std::vector<prolist_t> prog = file_to_list(dir + "/" + prog_file[i]);
			if (!prog.empty()) list.push_back(prog);
		}
		return list;
	}

	//ʱ��ת��
	struct tm*  getNowTime(int zone) {
		time_t now;
		time(&now);
		now += zone * 3600;
		return localtime(&now);
	}
};
