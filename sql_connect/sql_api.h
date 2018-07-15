#ifndef _SQL_
#define _SQL_
#include <iostream>
#include <string>
#include <strings.h>
#include <mysql.h>
#include <string.h>
#include <stdlib.h>

using namespace std;

class sqlApi{
	public:
		sqlApi(const std::string &_h="127.0.0.1",
				const int _port=3306,
				const std::string &_u="root",
				const std::string &_p="",
				const std::string& _db="http");
		~sqlApi();
		int connect();
		int insert(const std::string& _name,
				   const std::string& _sex,
				   const std::string& _age,
				   const std::string& _hobby,
				   const std::string& _school);
		int select();
	private:
		MYSQL* conn;
		MYSQL_RES * res;
		std::string host;
		std::string user;
		std::string passwd;
		std::string db;
		int port;
};

#endif
