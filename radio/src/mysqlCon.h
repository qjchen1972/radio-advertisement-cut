#pragma once

#ifndef _WIN32
#include <mysql/mysql.h>
#else
#include "mysql.h"
#endif
#include <string>
#include <vector>
#include <iostream>

#define MAX_STRSIZE  10240


/*��Ŀ��*/
struct prolist_t
{
	int ID;//��Ŀ���
	std::string name;//��Ŀ��
	std::string fileName;//�洢��linuxӲ���е��ļ���
	std::string pTime;//��Ŀ��ʼ����ʱ�䣬����06:00-07:00
	int startTime;//��ʼʱ��
	int endTime;//����ʱ��
	int startWriteTime;
	int endWriteTime;	
	int proStatus;//�Ƿ�Ϊ�����ݵĽ�Ŀ
	std::string  host;
};

class MysqlCon
{
public:
	MysqlCon(std::string stat) {
		radioName = stat;
	}
	~MysqlCon() {}
	
	bool insertInfo(prolist_t list, std::string proDate) {	

		MYSQL *con = mysql_init(NULL);
		if (!con) {
			std::cout << "Error:mysql_init::" << mysql_error(con) << std::endl;
			return false;
		}
		
		if (mysql_real_connect(con, dbip.c_str(), dbuser.c_str(), dbpwd.c_str(), dbname.c_str(), dbport, NULL, 0)) {
			if (mysql_select_db(con, dbname.c_str())) {
				std::cout << "Error:Select the database error" << std::endl;
				return false;
			}
			con->reconnect = 1;
		}
		else{
			std::cout << "Error:Unable to connect the database" << mysql_error(con) << std::endl;
			return false;
		}

		int proPlayTime = list.endWriteTime - list.startWriteTime;
		

		std::string t1 = std::to_string(list.startTime);
		std::string t2 = std::to_string(list.endTime);
		std::string t3 = std::to_string(proPlayTime);

		if (mysql_set_character_set(con, "GBK")) {
			printf("Failed to set character!Error::%s",mysql_error(con));
		}
		std::string proFilePath = radioName + "/mp3/" + proDate + "/" + list.fileName + ".mp3";
		std::string updateQuery = "insert into prolist(proName,proDate,proStart,proEnd,proTime,proPlayTime,proHost,proRadioName,proFileName,proFilePath)";
		updateQuery += "values ('" + list.name + "','" + proDate + "'," + t1 + "," + t2 + ",'" + list.pTime + "'," + t3 + ",'" + list.host + "','" + radioName + "','" + list.fileName + "','" + proFilePath + "');";
		
		int rt;

		//std::cout << updateQuery << std::endl;

		rt = mysql_real_query(con, updateQuery.c_str(), updateQuery.length());

		mysql_close(con);
		if (rt)	{
			printf("Error making query: %s !!!\n", mysql_error(con));
			return false;
		}
		return true;
	}

	bool selectInfo() {

		std::string query = "select * from prolist;";
		MYSQL *con = mysql_init(NULL); 
		if (!con) {
			std::cout << "Error:mysql_init::" << mysql_error(con) << std::endl;
			return false;
		}

		int rt;

		if (mysql_real_connect(con, dbip.c_str(), dbuser.c_str(), dbpwd.c_str(), dbname.c_str(), dbport, NULL, 0)) {
			if (!mysql_select_db(con, dbname.c_str())) {
				std::cout << "Select successfully the database!" << std::endl;
				con->reconnect = 1;
			}
		}
		else {
			std::cout << "Error:Unable to connect the database" << mysql_error(con) << std::endl;	
			return false;
		}

		if (mysql_query(con, query.c_str())){
			std::cout << "select error" << std::endl;
			mysql_close(con);
			return false;
		}
		else {

			MYSQL_RES *res = mysql_store_result(con);
			if (res) {

				int  num_fields = mysql_num_fields(res);   //��ȡ��������ܹ����ֶ�����������
				int  num_rows = mysql_num_rows(res);       //��ȡ��������ܹ�������
				
				for (int i = 0; i < num_rows; i++) //���ÿһ��
				{
					//��ȡ��һ������
					MYSQL_ROW row = mysql_fetch_row(res);
					if (row < 0) break;
					for (int j = 0; j < num_fields; j++)  //���ÿһ�ֶ�
					{
						std::cout << row[j] << "\t\t";
					}
					std::cout << std::endl;
				}
				mysql_free_result(res);
				mysql_close(con);
				return true;
			}
			else {
				std::cout << "û�ж�ȡ�����ݣ�" << std::endl;
				mysql_free_result(res);
				mysql_close(con);
				return false;
			}
		}
	}

private:	
	const std::string dbuser = "php_user";
	const std::string dbpwd = "hq123456";
	const std::string dbip = "127.0.0.1";
	const std::string dbname= "program";
	const std::string tbname= "prolist";
	std::string radioName;// = { "1300","1430","900","1480","1380" };
	const int dbport = 3306;
	//int radio;
};