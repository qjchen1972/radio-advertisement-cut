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


class MysqlCon
{
public:
	MysqlCon() {}
	~MysqlCon() {}
	
	bool insertInfo( std::string sqlstr) {	

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
		
		if (mysql_set_character_set(con, "GBK")) {
			printf("Failed to set character!Error::%s",mysql_error(con));
		}
		
		printf("sql %s \n", sqlstr.c_str());
		int rt = mysql_real_query(con, sqlstr.c_str(), sqlstr.length());
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

				int  num_fields = mysql_num_fields(res);   //获取结果集中总共的字段数，即列数
				int  num_rows = mysql_num_rows(res);       //获取结果集中总共的行数
				
				for (int i = 0; i < num_rows; i++) //输出每一行
				{
					//获取下一行数据
					MYSQL_ROW row = mysql_fetch_row(res);
					if (row < 0) break;
					for (int j = 0; j < num_fields; j++)  //输出每一字段
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
				std::cout << "没有读取到数据！" << std::endl;
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
	const int dbport = 3306;
};