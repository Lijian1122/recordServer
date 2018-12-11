#ifndef __http__
#define __http__
 
/*TODO : 连接失败后的特殊处理*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/wait.h>
#include <error.h>
#include <time.h>
#include <iostream>
 
 
using namespace std;
 
#define HTTP_KEEP_ALIVE "Keep-Alive"
#define HTTP_GET        "GET"
 
class http_request_header
{
	protected:
		std::string method; /*GET OPTION HEAD POST PUT DELETE TRACE CONNECT*/  //就是方法，但是我现在只实现了GET方法
		std::string file;	//目前只实现了GET方法,这里也就是  www.baidu.com/?abc 大概意思就是file = "?abc"
		std::string host;   /*这里是纯IP*/
		//CRLF
 
	private:
		/*这里在初始化的时候定义了一些常量,以后的程序可能会改*/
		std::string version;   /*默认就是 HTTP1.1 */
		std::string connection; /*  默认是Keep-Alive*/
		std::string space;  //就是空格
		std::string CRLF;   //就是回车，换行符。
 
	public:
		void set_method(const std::string &method);
		void set_file(const std::string &file);
		void set_host(const std::string &host);
		void set_connection(const std::string &connection);
 
		http_request_header(); //初始化函数，也就是初始化private里东西
		~http_request_header();
		std::string make();		//根据当前的数据，生成一个header
 
};
 
class Httpclient
{
	private:

	  
		void http_client_create(const std::string &host, const int port);	//创建一个客户机，参数分别形如 "192.168.1.1"和80这样的两个参数。第一个是IP，第二个是端口号
 
		void http_close();						    //断开和服务器链接
 
		int http_recv(std::string &ret);					//接受服务器返回数据（比如get请求有返回数据）
										//函数返回值为最后一次调用recv函数的返回值。
										//如果3秒没有接受到消息，则会自动退出


		int http_client_send(std::string msg);				//根据当前的header设置，向服务器发送一个msg信息。（会自动根据设置，生成头）
										//返回值和send函数意义相同。
	public:
		Httpclient();
		~Httpclient();

		int http_get(const std::string host_addr, const std::string file, const int port, std::string &ret);	//向host_addr发送一个http请求，其中的参数是file，端口号port。  返回的字符串为ret
													//这个函数的返回值，-1为失败，并会在标准输出上输出错误信息。否则返回1。
        char *rec_buff;			   //接受数据的buff

	private:
		int socket_fd;							//socket常用
		//char rec_buff[1024];				   //接受数据的buff
		std::string msg;							   //临时变量
		http_request_header hrh;			  //http请求的头部。
 
};

#endif