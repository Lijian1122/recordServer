#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/vfs.h>
#include <sys/syscall.h>

#include <dirent.h>
#include <iostream>
#include <map>

#include "glog/logging.h"
#include "mongoose.h"
#include "CThreadPool.h"
#include "MergeRunable.h"

#include "json.hpp"
#include "LibcurClient.h"
#include "message_queue.h"

using json = nlohmann::json;

#define CONTENTTYPE "Content-Type: application/x-www-form-urlencoded\r\n"

string  AACSTR = ".aac";
string  H264STR = ".h264";
string  JSONSTR = ".json";
string  MERGESTR = "merge.";

//string  RELATIVEPATH= "/home/record_server/recordFile/";
string  RELATIVEPATH= "./s22/";

string  IPPORT= "http://192.168.1.205:8080/live/";

string  serverName = "合成服务105";

static const char *s_http_port = "8081";

static int merge_serverId = 0;  //录制服务ID

LibcurClient *m_httpclient, *s_httpclient;


typedef pair<int , string> PAIR;

ostream& operator<<(ostream& out, const PAIR& p) 
{
  return out << p.first << "\t" << p.second;
}

CThreadPool *threadpool;
pthread_t httpServer_t, httpTime_t;
int httpSev_flag = 1;

int merge_flag = 1; //服务在线状态上传标志

//http监听服务线程处理数据
void ev_handler(struct mg_connection *nc, int ev, void *ev_data) 
{
      switch (ev) 
      {
         case MG_EV_ACCEPT: 
         {
            char addr[32];
            mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
            break;
        }
        case MG_EV_HTTP_REQUEST: 
        {

             struct http_message *hm = (struct http_message *) ev_data;
             char addr[32];
             mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
             printf("%p: %.*s %.*s %.*s\r\n", nc, (int) hm->method.len, hm->method.p,
             (int) hm->uri.len, hm->uri.p ,(int) hm->body.len, hm->body.p);
    
             char url[(int)hm->uri.len];
             sprintf(url, "%.*s",(int)hm->uri.len,hm->uri.p);

             printf("get url:%s \n",url);
             LOG(INFO) << "Get Url:"<<url;
   
             char urls[11];
             sprintf(urls, "%.*s",11,hm->uri.p);

             int ret = 0;
             if(strcmp(url, "/live/merge") == 0)
             {
         
                 printf("start: %s\n" , url);
             
                 //处理参数
                 char parmStr[(int) hm->query_string.len];
                 sprintf(parmStr, "%.*s",(int) hm->query_string.len,hm->query_string.p);
          
                 char liveId[128] = {0};
                 mg_get_http_var(&hm->query_string, "liveId", liveId, 128);//获取liveID

                 printf("参数2 : %s\n",liveId);
       
                 if(strlen(liveId) == 0)
                 {
                    printf("%s\n","liveld is empty");
                    LOG(ERROR)<<"liveld is empty";

                    ret = 3;
                    goto end;
                 }

                 string parm = RELATIVEPATH;
                 parm += liveId;

                 LOG(INFO)<<"获合成参数 livID:"<<liveId;
                 MergeRunable *taskObj = new MergeRunable(parm);
                 threadpool->AddTask(taskObj);       
     }
     
  end:
          char numStr[10] ={};
          snprintf(numStr, sizeof(numStr), "%d",ret);
     
          //组装json返回值
          json obj;
          obj["resCode"] = numStr;
          std::string resStr = obj.dump();

          mg_send_response_line(nc, 200,
                            "Content-Type: text/html\r\n"
                            "Connection: close");        

          mg_printf(nc,"\r\n%s\r\n",resStr.c_str());
    
          nc->flags |= MG_F_SEND_AND_CLOSE;

         break;
     }
    
    case MG_EV_CLOSE:
    {
        
         printf("%p: Connection closed\r4\n", nc); 
         break;
    }
  }
}


//http服务监听线程
void *httpServer_fun(void *pdata)
{
   struct mg_mgr mgr;
   struct mg_connection *nc;

   mg_mgr_init(&mgr, NULL);
  
   nc = mg_bind(&mgr, s_http_port, ev_handler);
   if (nc == NULL) 
   {
      printf("Failed to create listener!\n");
      LOG(ERROR)<<"Failed to create listener";
      httpSev_flag = 0;
      pthread_detach(pthread_self());
      return 0;
   }

   printf("合成服务已启动 on port: %s\n" ,s_http_port); 
   LOG(INFO) <<"合成服务已启动 on port:"<<s_http_port;

   mg_set_protocol_http_websocket(nc);

   while(httpSev_flag)
   {    
      mg_mgr_poll(&mgr, 1000);
   }
   return pdata;
}

int CreateLogFileDir(const char *sPathName)  
{  
      char DirName[256];  
      strcpy(DirName, sPathName);  
      int i,len = strlen(DirName);
      for(i=1; i<len; i++)  
      {  
          if(DirName[i]=='/')  
          {  
              DirName[i] = 0; 
              if(access(DirName, F_OK)!=0)  
              {  
                   
                  if(mkdir(DirName, 0755)==-1)  
                  {   
                      printf("mkdir log error\n");  
                      LOG(ERROR)<<"mkdir log error"; 
                      return -1;   
                  }  
                 
              }  
              DirName[i] = '/';  

          }  
      }  
      return 0;  
} 


//定时上传状态线程
void *httpTime_fun(void *pdata)
{
   s_httpclient = new LibcurClient;

   string url = IPPORT;
   url = url.append("server_update?serverId=");
   char numStr[1024] ={};
   snprintf(numStr, sizeof(numStr), "%d",merge_serverId);
   url.append(numStr);
   url.append("&netFlag=20");
   cout<<"time url:"<<url<<endl;

   //录制服务在线状态定时上传
   while(merge_flag)
   {
      sleep(10);

      int main_ret = s_httpclient->HttpGetData(url.c_str());
      if(main_ret != 0)
      {
         LOG(ERROR) << "定时上传服务状态失败 "<<"错误代号:"<<main_ret;  

      }else
      {
         json m_object = json::parse(s_httpclient->GetResdata());
         if(m_object.is_object())
         {
             string resCode = m_object.value("code", "oops");
             main_ret = atoi(resCode.c_str() );

             if(0 != main_ret)
             {
                std::cout<<main_ret<<endl;
                LOG(ERROR) << "定时上传服务状态失败 main_ret:"<< main_ret <<" "<<"错误信息:"<<m_object.at("msg");   
             }
         }
      }
    }

   if(NULL != s_httpclient)
   {
      delete s_httpclient;
      s_httpclient = NULL;
   }

   printf("stop timer\n");

   return pdata;
}

int stopServer()
{

//     int main_ret = 0;
//     char stopStr[1024] = {0};

// here:
//    scanf("%s",stopStr);

//    if(strcmp("stop",stopStr) == 0)
//    {

//        httpSev_flag = 0;
//        main_ret = pthread_join(httpServer_t, NULL);
//        if(0 != main_ret)
//        {
//          printf("http服务线程退出错误  ret:%d" ,main_ret); 
//          LOG(ERROR)<<"http服务线程退出错误  ret:"<<main_ret; 
//        } 
 
//        LOG(INFO)<<"http服务线程已退出 ret:"<<main_ret; 

//        while(1)
//        {
//           if(threadpool->getTaskSize()==0) 
//           {
//              if(0 == threadpool->StopAll())
//              {
//                 break;
//              }else
//              {
//              	LOG(ERROR)<<"线程池终止所有线程错误!!"; 
//              } 
//           }
//       }  
      
//       LOG(INFO)<<"线程池 所有线程已终止!"; 
//       if(NULL != threadpool)
//       {
//          delete threadpool;
//          threadpool = NULL;
//       }      
//       return main_ret;

//    }else
//    {   
//        printf("请输入正确的停止命令!\n");
//        memset(stopStr, 0 ,1024);
//        goto here;
//    }

  return 0;
}

int main(int argc, char* argv[]) 
{
   int main_ret = 0;
   m_httpclient = new LibcurClient;

   printf("monitor_Server :[tid: %ld]\n", syscall(SYS_gettid));

   main_ret = CreateLogFileDir("./mergeLog/");
   if(0 != main_ret)
   {
       LOG(ERROR) << "创建log 文件夹失败"<<" "<<"main_ret:"<<main_ret;
       return main_ret ;
   }

   //创建log初始化
  google::InitGoogleLogging("");
  google::SetLogDestination(google::GLOG_INFO,"./mergeLog/mergeServer-");
  FLAGS_logbufsecs = 0; //缓冲日志输出，默认为30秒，此处改为立即输出
  FLAGS_max_log_size = 500; //最大日志大小为 100MB
  FLAGS_stop_logging_if_full_disk = true; //当磁盘被写满时，停止日志输出


   //注册服务接口  
  // string url = IPPORT;
  // url.append("server_create?serverType=2&serverName=");
  
  // char *format = m_httpclient->UrlEncode(serverName);
  // url.append(format);
  // url.append("&netFlag=1&serverIp=192.168.1.206:8000"); 

  // printf("%s\n", url.c_str());

  // main_ret = m_httpclient->HttpGetData(url.c_str());
  // if(main_ret != 0)
  // {

  //   cout<<main_ret<<endl;
  //   LOG(ERROR) << "注册录制服务失败:  "<<"错误代号:"<<main_ret;
  //   return main_ret;

  // }else
  // {
  //   std::string res = m_httpclient->GetResdata();
  //   printf("777:%s\n", res.c_str());
  //   json m_object = json::parse(m_httpclient->GetResdata());
  //   if(m_object.is_object())
  //   {
  //     string resCode = m_object.value("code", "oops");
  //     main_ret = atoi(resCode.c_str() );

  //     if(0 != main_ret)
  //     {
  //         std::cout<<main_ret<<endl;
  //         LOG(ERROR)<<"注册录制服务失败 main_ret:"<< main_ret <<" "<<"错误信息:"<<m_object.at("msg");
  //         return main_ret ;
  //     }else
  //     {
  //         merge_serverId = m_object.value("serverId", 0);
  //         std::cout<<merge_serverId<<endl;
  //     }
    
  //   }
  // }
 
  //往消息队列里面写数据
  int msqid = getMsgQueue();
  char numStr[1024] ={0};
  snprintf(numStr, sizeof(numStr), "%d",merge_serverId);
  sendMsg(msqid, CLIENT_TYPE, numStr);


  //创建http服务线程  
  main_ret = pthread_create(&httpServer_t, NULL, httpServer_fun, NULL);

  if(0 != main_ret)
  { 
    printf("http服务监听线程创建失败 ret:%d" ,main_ret); 
    LOG(ERROR) << "http服务监听线程创建失败"<<" "<<"main_ret:"<<main_ret;
    return main_ret;   
  }

  threadpool = new CThreadPool(10); //线程池大小为10


   //创建定时上传线程
   main_ret = pthread_create(&httpTime_t,NULL, httpTime_fun, NULL);
   if(0 != main_ret)
   {
     LOG(ERROR) << "http定时上传线程创建失败"<<" "<<"main_ret:"<<main_ret; 
     return main_ret;   
   }


  main_ret = pthread_join(httpServer_t, NULL);
  if(0 != main_ret)
  {
      printf("http服务线程退出错误  ret:%d" ,main_ret); 
      LOG(ERROR)<<"http服务线程退出错误  ret:"<<main_ret; 
  } 

  //main_ret = stopServer();

  printf("合成服务已退出 ret:%d\n" ,main_ret);
  LOG(INFO) << "合成服务已退出"<<" "<<"main_ret:"<<main_ret;

  return main_ret;
}
