#pragma once


#include <string>
#include <string.h>
#include <vector>
#include <Urlmon.h>
#include "mysql/jdbc.h"
#include "jdbc/mysql_connection.h"
#include "jdbc/mysql_driver.h"
#include "jdbc/mysql_error.h"
#include <cwchar>
struct prolist_t
{
	int ID;//节目编号
	std::string name;//节目名
	std::string pTime;//节目开始结束时间，例：06:00-07:00
	int startTime;//开始时刻
	int endTime;//结束时刻
	int playTime;//播放时长
	int proStatus;//是否为有内容的节目
};

class mysqlCon
{
public:
	mysqlCon() {
		user = "root";
		pwd = "123";
		host = "192.168.127.128:3306";
		database = "stu";
	}
	~mysqlCon() {}

	int executeUpdate(prolist_t list, std::string proDate, std::string proHost) {

		sql::Driver *driver;
		sql::Connection *con;
		sql::Statement *stmt;

		try
		{
			driver = get_driver_instance();
			con = driver->connect(host.c_str(), user.c_str(), pwd.c_str());
			con->setSchema(database.c_str());

			stmt = con->createStatement();
			//stmt->executeQuery("set names utf8");//设置编码格式
			int proPlayTime = 0;
			if (list.endTime <= 0) {
				proPlayTime = 60 * 24 - list.startTime;
			}
			else {
				proPlayTime = list.endTime - list.startTime;
			}

			std::string t1 = std::to_string(list.startTime);
			std::string t2 = std::to_string(list.endTime);
			std::string t3 = std::to_string(proPlayTime);
			proHost = "";
			//CChineseCode cc;

			std::string updateQuery = "insert into prolist(proName,proDate,proStart,proEnd,proTime,proPlayTime,proHost)";
			updateQuery += "values ('" + list.name + "','" + proDate + "'," + t1 + "," + t2 + ",'" + list.pTime + "'," + t3 + ",'" + proHost + "');";
			std::cout << updateQuery << std::endl;
			updateQuery;
			
			std::wstring wstrResult = String_To_WString(updateQuery);
			char *utf = Unicode_To_Utf8(wstrResult.c_str());

			if (stmt->executeUpdate(utf)) {
				//std::cout << "插入成功！" << std::endl;
				con->close();
				delete stmt;
				delete con;
				return 1;
			}
			else
			{
				//std::cout << "插入失败！" << std::endl;
				con->close();
				delete stmt;
				delete con;
				return 0;
			}


		}
		catch (sql::SQLException &e)
		{
			throw e;
			
		}
	}

private:
	std::string user;
	std::string pwd;
	std::string host;
	std::string database;


	wchar_t* Utf8_To_Unicode(char* row_i)
	{
		int len = MultiByteToWideChar(CP_UTF8, 0, row_i, strlen(row_i), NULL, 0);
		wchar_t *wszStr = new wchar_t[len + 1];
		MultiByteToWideChar(CP_UTF8, 0, row_i, strlen(row_i), wszStr, len);
		wszStr[len] = '\0';
		return wszStr;
	}

	char* Unicode_To_Utf8(const wchar_t* unicode)
	{
		int len;
		len = WideCharToMultiByte(CP_UTF8, 0, unicode, -1, NULL, 0, NULL, NULL);
		char *szUtf8 = (char*)malloc(len + 1);
		memset(szUtf8, 0, len + 1);
		WideCharToMultiByte(CP_UTF8, 0, unicode, -1, szUtf8, len, NULL, NULL);
		return szUtf8;
	}

	std::wstring String_To_WString(const std::string& s)
	{
		std::string strLocale = setlocale(LC_ALL, "");
		const char* chSrc = s.c_str();
		size_t nDestSize = mbstowcs(NULL, chSrc, 0) + 1;
		wchar_t* wchDest = new wchar_t[nDestSize];
		wmemset(wchDest, 0, nDestSize);
		mbstowcs(wchDest, chSrc, nDestSize);
		std::wstring wstrResult = wchDest;
		delete[]wchDest;
		setlocale(LC_ALL, strLocale.c_str());
		return wstrResult;
	}
};
