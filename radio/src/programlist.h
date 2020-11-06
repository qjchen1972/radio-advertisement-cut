#pragma once

#include <string>
#include <vector>
#include <time.h>
#include <iostream>
#include <fstream>
#include <thread>
#include <sys/stat.h> 
#include <sys/types.h>

#include"mysqlCon.h"


#define INIT  0
#define RESTARTING 1
#define RECORDING  2



class ProgramList {

public:
	ProgramList(std::string dir,std::string radioName,int timeZone) {
		outDir = dir + "mp3/";
		proDir = dir + "list/";
		station = radioName;
		timezone = timeZone;
		status = INIT;
	}
	~ProgramList() {}


	//返回1时，表示重新连接
	int  saveFile(char* buf, int len) {
		bool needConnect = false;
		if (!getSaveFileHandle(needConnect)) return 0;
		if (needConnect)  return 1;
		if (!isStartWirte(curIndex)) return 0;
		curFileHandle.write(buf, len);
		if (curFileHandle.bad())
			std::cout << curRadioList[curIndex].name << "  write error " << len << std::endl;
		return 0;
	}

	//音频文件路径处理
	int  getSaveFileHandle(bool &needConnect) {
		//0时刻，连接状态为非重启
		if (isZeroTime() && status != RESTARTING) {			

			//非初始化，关闭连接，否则继续
			if (status != INIT) {
				curFileHandle.close();
				writeDB(curRadioList[curIndex]);
				needConnect = true;
				std::cout << " FM"+ station +": "+ subOutDir + "'s NO." << curIndex << "( " << curRadioList[curIndex].pTime << " ) succes download!" << std::endl;
			}
			
			//根据节目单打开当前时间的节目
			if (!getTodayList(curRadioList)) return  0;
			curIndex = 0;
			status = RESTARTING;
			if (!openRadioFile(curIndex)) return 0;
			return 1;
		}

		//非0时刻，连接状态为初始化
		if (status == INIT) {
			//获得今天的节目单
			if (!getTodayList(curRadioList)) return  0;
			curIndex = findNowProgam(curRadioList);
			//writeDB(curRadioList[curIndex]);
			if (curIndex < 0) return 0;
			status = RECORDING;
			if (!openRadioFile(curIndex)) return 0;
			return 1;
		}

		//如果到了当前节目的结束时间
		if (isOver(curIndex)) {
			if (curRadioList[curIndex].proStatus) {
				curFileHandle.close();
				writeDB(curRadioList[curIndex]);
				std::cout << " FM" + station + ": " + subOutDir + "'s NO." << curIndex << "( " << curRadioList[curIndex].pTime << " ) succes download!" << std::endl;
			}
			curIndex++;
			if (curIndex == 1) status = RECORDING;
			if (!openRadioFile(curIndex)) return 0;
			needConnect = true;
			return 1;
		}
		if (!curRadioList[curIndex].proStatus) return 0;
		return 1;
	}


	//获得今天的节目单
	int  getTodayList(std::vector<prolist_t> &list) {

		std::string fileName = getTodayFile();
		//判断根据节目的名称获得的txt文件是否为空
		if (fileName.empty()) return 0;

		std::ifstream of;
		of.open(fileName, std::ios::in);
		if (!of.is_open()) {
			std::cout << fileName << " open err " << std::endl;
			return 0;
		}

		//根据日期创建文件目录
		if (!createDirs()) {
			std::cout << "Create outDir failed!" << std::endl;
			return 0;
		}

		int num = 0;
		list.clear();
		while (!of.eof()) {
			prolist_t node;
			std::string strTime;//临时获得时间的字符串

			of >> node.name >> node.pTime >> node.proStatus >> node.startWriteTime >> node.endWriteTime >> node.host;
			if (of.fail()) break;

			strTime = node.pTime;
			int key, value;
			//std::string p1 = strTime.substr(0, 5);
			//std::string p2 = strTime.substr(6);
			if (sscanf(strTime.substr(0, 5).c_str(), "%d:%d", &key, &value) != 2) {
				of.close();
				return 0;
			}
			node.startTime = key * 3600 + value * 60;//精确到秒
			node.startWriteTime = node.startTime + node.startWriteTime;

			if (sscanf(strTime.substr(6).c_str(), "%d:%d", &key, &value) != 2) {
				of.close();
				return 0;
			}

			node.endTime = key * 3600 + value * 60;
			if (node.endTime == 0)
				node.endTime = 24 * 3600;

			node.endWriteTime = node.endTime - node.endWriteTime ;
			//1分钟增加1秒的延时
			node.endWriteTime += (node.endWriteTime - node.startWriteTime) / 60;

			node.fileName = station + "_" + subOutDir + "_" + std::to_string(num);

			node.ID = num++;

			list.push_back(node);
		}
		of.close();
		

		return 1;
	}

	//获得今天的节目单名称
	std::string getTodayFile() {
		std::string temp;
		struct tm *pt = getNowTime(timezone);
		if (!pt || pt->tm_wday > 7 || pt->tm_wday < 0)  return temp;
		return proDir + proFile[pt->tm_wday];
	}


private:
	int status = INIT;
	int timezone;// 相对于当前时区的时间差 洛杉矶时间比北京时间晚了15个小时
	std::string proDir;
	std::string proFile[7] = {"Sun.txt","Mon.txt","Tues.txt","Wed.txt","Thur.txt","Fri.txt","Sat.txt"};
	std::string  outDir;
	std::string  subOutDir;
	int curIndex;
	std::vector<prolist_t> curRadioList;
	std::ofstream curFileHandle;
	std::string station;

	
	//时区转换
	struct tm*  getNowTime(int zone) {
		time_t now;
		time(&now);
		now += zone * 3600;
		return localtime(&now);
	}

	//是否为00:00时刻
	bool isZeroTime() {
		struct tm *pt = getNowTime(timezone);
		if (pt->tm_hour == 0 && pt->tm_min < 1) return true;
		return false;
	}

	//创建文件目录
	bool createDirs()
	{
		struct tm *pt = getNowTime(timezone);
		char timestr[128];
		sprintf(timestr,"%04d-%02d-%02d", (1900 + pt->tm_year), (1 + pt->tm_mon), (pt->tm_mday));
		subOutDir = timestr;
		if (!mkdir((outDir +subOutDir).c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO)) return true;
		else {
			if (errno == EEXIST) return true;
			return false;
		}
	}

	//获得当前时间的节目的路径
	std::string getTimeFileName(std::string fileName) {
		struct tm *pt = getNowTime(timezone);
		return  outDir + subOutDir + "/" + fileName + ".mp3";
	}

	//获得当前时间判断节目是否结束
	bool isOver(int index) {
		struct tm *pt = getNowTime(timezone);
		if (pt->tm_hour * 3600 + pt->tm_min * 60 + pt->tm_sec >= curRadioList[index].endWriteTime) return true;
		return false;
	}

	//获得当前时间判断节目是否开始写
	bool isStartWirte(int index) {
		struct tm *pt = getNowTime(timezone);
		if (pt->tm_hour * 3600 + pt->tm_min * 60 + pt->tm_sec >= curRadioList[index].startWriteTime) return true;
		return false;
	}

	bool openRadioFile(int index) {

		//如果当前时间段为没有价值的节目
		if (!curRadioList[index].proStatus) {
			//std::cout << std::endl << curRadioList[curIndex].name << "不打开！" << std::endl;
			return false;
		}
		curFileHandle.open(getTimeFileName(curRadioList[index].fileName), std::ios::out | std::ios::app | std::ios::binary);
		//如果连接没有成功打开
		if (!curFileHandle.is_open())  return false;
		//std::cout << std::endl << curIndex << "  open succes " << std::endl;
		return true;
	}

	int findNowProgam(std::vector<prolist_t> list) {
		struct tm *pt = getNowTime(timezone);
		int now = pt->tm_hour * 3600 + pt->tm_min * 60 + pt->tm_sec;
		for (int i = 0; i < list.size(); i++) {
			if (now >= list[i].startTime && now <= list[i].endTime) return i;
		}
		return -1;
	}

	void writeDB(prolist_t  radio) {

		std::string str = subOutDir;
		std::string stat = station;
		auto temp_thread = std::thread([stat,radio, str]() {

			MysqlCon mysql(stat);
			mysql.insertInfo(radio, str);
		});
		temp_thread.detach();
	}
};
