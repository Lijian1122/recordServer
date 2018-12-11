#include "LibcurClient.h"

LibcurlFlag LibcurClient::m_lfFlag = Lf_Get;

LibcurClient::LibcurClient()
{
	
	curl_global_init(CURL_GLOBAL_ALL); // 首先全局初始化CURL
	m_curl = curl_easy_init(); // 初始化CURL句柄
	
	if(NULL == m_curl)
	{
		printf("(申请内存失败!\n");

		curl_easy_cleanup(m_curl);
	}

	m_getWritedata = "";
}


//get请求方法
int LibcurClient::HttpGetData(const char *url, int timeout , int connect_timeout)
{
	//判断是否申请内存成功
	if(m_curl == NULL)
	{
		printf("%s", "初始化失败!\n");
		curl_easy_cleanup(m_curl);
		return -1;
	}

	m_getWritedata = "";

	m_lfFlag = Lf_Get;

	// 设置目标URL
	CURLcode res = curl_easy_setopt(m_curl, CURLOPT_URL, url);

	// 设置接收到HTTP服务器的数据时调用的回调函数
	res = curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writeCallbackData);
	// 设置自定义参数(回调函数的第四个参数)

	res = curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this); //&m_getWritedata

	//支持重定向
	res = curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1);

	//设置数据返回超时时间为30s
	res = curl_easy_setopt(m_curl, CURLOPT_TIMEOUT, timeout);

	//设置连接超时时间为10s
	res = curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT, connect_timeout);

	// 执行一次URL请求
    res = curl_easy_perform(m_curl);

	return res;

}


//写结果回调
size_t  LibcurClient::writeCallbackData(void *buffer, size_t size, size_t nmemb, void *stream)
{
	int i = 0;
	if (stream) 
	{
		switch (m_lfFlag)
		{		

		case Lf_Get:
		     {
			   LibcurClient* pThis = (LibcurClient*)stream;
	           return pThis->WriteCallbackData(buffer, size, nmemb);
		     }
			 break;
		case Lf_Post:
			 break;	
	    default:
			break;
		}	

		return size * nmemb;
	}
	else 
	{
		return size * nmemb;
	}
}

//post请求方法
int LibcurClient::HttpPostData(const char *url, const char * localpath, const char * postParams, int timeout, int connect_timeout)
{
	return 0;
}

size_t LibcurClient::WriteCallbackData(void *buffer, size_t size, size_t nmemb)
{
    char* pData = (char*)buffer;

	m_getWritedata.append(pData, size * nmemb);

    //cout<<"shuju:"<<m_getWritedata<<endl;
	return size * nmemb;
}

//返回数据对外接口
std::string LibcurClient::GetResdata()
{
   return m_getWritedata;
}

char *LibcurClient::UrlEncode(std::string &m_str)
{
	return curl_easy_escape(m_curl ,m_str.c_str() ,static_cast<int>(m_str.size()));
}

LibcurClient::~LibcurClient()
{

	if(m_curl)
	{
		curl_easy_cleanup(m_curl);
	}

	curl_global_cleanup();
}
