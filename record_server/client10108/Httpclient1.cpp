#include "Httpclient.h"

void http_request_header::set_method(const std::string &method)
{
	this -> method = method;
}
 
void http_request_header::set_file(const std::string &file)
{
	this -> file = file;
}
 
void http_request_header::set_host(const std::string &host)
{
	this -> host = host;
}
 
void http_request_header::set_connection(const std::string &connection)
{
	this -> connection = connection;
}
 
http_request_header::http_request_header()
{
	version = std::string("HTTP/1.1");
	CRLF = std::string("\r\n");
	space = std::string(" ");

    connection =std::string("");
	method = std::string(""); 
    file = std::string("");
    host = std::string(""); 
}

http_request_header::~http_request_header()
{

}


std::string http_request_header::make()
{
	std::string ret = method;
	if(method.compare(std::string("GET"))==0)
	{
		ret += std::string(" /") + file + space + version + CRLF\
		       +  std::string("HOST:") + space + host + CRLF\
		       +  std::string("Connection:")  + space + connection + CRLF\
		       +  CRLF;
	}
	else
	{
		printf("error : is not the GET request!?\n");
		exit(0);
	}
	return ret;
}
 
/*-------------------http_request_header end-----------------------------*/
/*-----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------*/
/*-----------------------------------------------------------------------*/
/*-----------------------httpclient start--------------------------------*/
 
Httpclient::Httpclient()
{
     socket_fd = 0;	
     msg = std::string("");	

     rec_buff = (char*)malloc(sizeof(char)*1024);
}
void Httpclient::http_client_create(const std::string &host, const int port)
{
	//struct hostent *he = NULL;
	struct sockaddr_in server_addr;
 
 
	//cout << host <<" " << port << endl;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(host.c_str());//*((struct in_addr *)he -> h_addr);
 
 
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1 )
	{
 
		printf("error: creat socket error\n");
		exit(0);
		//return -1;
	}
 
	if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1)
	{
		printf("error: connect socket error\n");
		exit(0);
		//return -1;
	}
 
}
 
void Httpclient::http_close()
{
	close(socket_fd);
}
 
int Httpclient::http_recv(std::string &ret)
{
	int recvnum = 0;
	ret = "";
	memset(rec_buff, 0, sizeof(rec_buff));
	while (	recvnum = recv(socket_fd, rec_buff, sizeof(rec_buff), 0) > 0)
	{
		ret += std::string(rec_buff);
		memset(rec_buff, 0, sizeof(rec_buff));
	}
	return recvnum;
}
 
int Httpclient::http_client_send(std::string msg)
{
	msg = hrh.make() + msg;
	int flag = send(socket_fd, msg.c_str(), msg.size() ,0);
	if (flag < 0)
	{
		printf("send socket error\n");
	}
	return flag;
}
 
int Httpclient::http_get(const std::string host_addr, const std::string file, const int port, std::string &ret)
{
	std::string host;
 
	if(host_addr.size() == 0)
	{
		printf("error: url's length error \n");
		return -1;
	}
 
	if (host_addr.find("http://", 0) == -1)
	{
		printf("warnning ! the host_addr is not the http ?\n");
		host = host_addr;
	}
	else
	{
		host = host_addr.substr(7, host_addr.size() - 7);
	}
 
	http_client_create(host ,port);
 
	if(socket_fd < 0)
	{
		printf("error : http_lient_create failed\n");
		return -1;
	}
 
	//make the http header
	hrh.set_method(HTTP_GET);
	hrh.set_file(file);
	hrh.set_host(host);
	hrh.set_connection(HTTP_KEEP_ALIVE);
	msg="";
 
	if( http_client_send(msg) < 0)
	{
		printf("error : http_client_send failed..\n");
		return -1;
	}
 
	struct timeval timeout = {1, 0};
	int flag = 0;
	setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(struct timeval));
	flag= http_recv(ret);
	http_close();
	return 1;
}

Httpclient::~Httpclient()
{
   if(NULL != rec_buff)
   {
   	  free(rec_buff);
   	  rec_buff = NULL;
   }
}