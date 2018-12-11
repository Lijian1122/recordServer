
#ifndef LIBCURCLIENT_H
#define LIBCURCLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <curl/curl.h> 
#include <curl/easy.h>
#include <iostream>

//请求类型
enum LibcurlFlag
{
	Lf_None = 0,
	Lf_Get,
	Lf_Post
};

using namespace std;

class LibcurClient
{
public:
	
	LibcurClient();

	~LibcurClient();

	//get请求方法
	int HttpGetData(const char *url, int timeout = 10, int connect_timeout = 10);

	//post请求方法
	int HttpPostData(const char *url, const char * localpath, const char * postParams= NULL, int timeout = 10, int connect_timeout = 10);

	//写结果回调
	static size_t  writeCallbackData(void *buffer, size_t size, size_t nmemb, void *stream);

	size_t WriteCallbackData(void *buffer, size_t size, size_t nmemb);

	std::string GetResdata();

	char *UrlEncode(std::string &m_str);

public:

	static LibcurlFlag  m_lfFlag;

private:

    CURL* m_curl;
  
	std::string m_getWritedata;
};

#endif