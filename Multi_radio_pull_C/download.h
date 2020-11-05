#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <time.h>
#include "mysocket.h"
#include "programlist.h"
#include <thread>

#ifndef _WIN32
#include "mysqlCon.h"
#include <sys/stat.h> 
#include <sys/types.h>
#endif

class Downloader {

public:
	Downloader(std::string name, int type, int timezone, std::string filepath, std::string progpath){
		m_prog = new ProgramList(progpath);
		m_download_path = filepath;
		m_timezone = timezone;
		radio_name = name;
		radio_type = type;
	}
	~Downloader(){
		if(m_prog)	delete m_prog;
	}

	void show_progress(int len) {
		m_progress_len += len;
		if(m_progress_len >= 16 * 1024 * 60) {
			std::cout << "#";
			fflush(stdout);
			m_progress_len = 0;
		}
	}

	int write_file(const char* buf, int len) {
		
		if (!update_cur_handle()) return 0;
		m_curfile_handle.write(buf, len);
		if (m_curfile_handle.bad()) {
			std::cout << m_cur_filepath << "  write error " << len << std::endl;
			return 0;
		}
		//show_progress(len);
		return 1;		
	}	

	std::string radio_name;

private:
	ProgramList *m_prog = nullptr;
	std::string m_download_path;
	int m_timezone = 0;

	int m_cur_week = -1;
	int m_cur_id = -1;
	prolist_t m_cur_prog;
	std::string m_cur_day;
	std::string m_cur_filename;
	std::string m_cur_filepath;
	std::ofstream m_curfile_handle;

	int m_progress_len = 0;
	int radio_type;

	int update_cur_handle() {

		int week, id;
		prolist_t prog;
		m_prog->get_now_prog(m_timezone, week, id, prog);
		if (week != m_cur_week) {
			if (m_curfile_handle.is_open()) {
				m_curfile_handle.close();
				if (!radio_type)
					filetomp3(m_cur_filepath, m_cur_filepath + ".mp3");				
				printf("%s downloaded \n", m_cur_filepath.c_str());
				write_db();
			}

			m_cur_day = _today();
			_mkdir(m_download_path, m_cur_day);			
			m_cur_prog = prog;
			m_cur_week = week;
			m_cur_id = id;
			if (radio_type)
				m_cur_filename = prog.pTime + ".mp3";
			else
				m_cur_filename = prog.pTime;

			m_cur_filepath = m_download_path + "/" + m_cur_day + "/" + m_cur_filename;
			m_curfile_handle.open(m_cur_filepath, std::ios::out | std::ios::app | std::ios::binary);
			if (!m_curfile_handle.is_open())  return 0;
			printf("%s start download... \n", m_cur_filepath.c_str());
		}
		else if (m_cur_id != id) {
			m_curfile_handle.close();
			if (!radio_type)
				filetomp3(m_cur_filepath, m_cur_filepath + ".mp3");
			printf("%s downloaded \n", m_cur_filepath.c_str());
			write_db();
			m_cur_prog = prog;
			if (radio_type)
				m_cur_filename = prog.pTime + ".mp3";
			else
				m_cur_filename = prog.pTime;
			m_cur_filepath = m_download_path + "/" + m_cur_day + "/" + m_cur_filename;
			m_cur_id = id;
			m_curfile_handle.open(m_cur_filepath, std::ios::out | std::ios::app | std::ios::binary);
			if (!m_curfile_handle.is_open())  return 0;
			printf("%s start download... \n", m_cur_filepath.c_str());
		}
		else;
		return 1;
	}

	struct tm*  getNowTime(int zone) {
		time_t now;
		time(&now);
		now += zone * 3600;
		return localtime(&now);
	}

	std::string  _today() {
		struct tm *pt = getNowTime(m_timezone);
		std::string temp;
		char timestr[128];
		sprintf(timestr, "%04d-%02d-%02d", (1900 + pt->tm_year), (1 + pt->tm_mon), (pt->tm_mday));
		temp = timestr;
		return temp;
	}

	bool _mkdir(std::string dir, std::string name)
	{
#ifndef _WIN32
		if (!mkdir((dir + "/" + name).c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO)) return true;
		else {
			if (errno == EEXIST) return true;
			return false;
		}
#endif
		return true;
	}

	void filetomp3(std::string src, std::string dst) {

		auto temp_thread = std::thread([src, dst]() {
			std::string cmd = "ffmpeg -i " + src + " -y " + dst;
			system(cmd.c_str());
			cmd = "rm -rf " + src;
			system(cmd.c_str());
		});
		temp_thread.detach();
	}

	void write_db() {
#ifndef _WIN32
		auto temp_thread = std::thread([this]() {

			char buf[512];
			
			std::string filename = this->m_cur_filename; //+".mp3";
			std::string filepath = this->m_cur_filepath; //+".mp3";
			if (!this->radio_type) {
				filename += ".mp3";
				filepath += ".mp3";
			}

			sprintf(buf, "insert into prolist(proName,proDate,proTime,proHost,proRadioName,proFileName,proFilePath) \
			         values(\"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\", \"%s\");", 				
				    this->m_cur_prog.name.c_str(),
				    this->m_cur_day.c_str(),
  				    this->m_cur_prog.pTime.c_str(),
				    this->m_cur_prog.host.c_str(),
				    this->radio_name.c_str(),
				    filename.c_str(),
				    filepath.c_str());
			MysqlCon mysql;
			mysql.insertInfo(buf);
		});
		temp_thread.detach();
#endif
	}

};