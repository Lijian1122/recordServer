#include <string.h>
#include <stdlib.h> 

#include <dlfcn.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sched.h>
#include <iostream>
#include <map>
#include <list>
#include <queue>
#include <algorithm>
#include <termio.h>
#include <malloc.h>

#include <sys/select.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/syscall.h>

#include <signal.h>

#include "mongoose.h"
#include "glog/logging.h"
#include "RecordSave/RecordSaveRunnable.h"
#include "Httpclient/LibcurClient.h"
#include "message_queue.h"

#include "json.hpp"

#define CONTENTTYPE "Content-Type: application/x-www-form-urlencoded\r\n"  

using json = nlohmann::json;

using namespace std;

typedef struct finder_t
{
  finder_t(char *n)
    : rID(n)
  {
  }
  bool operator()(RecordSaveRunnable *p)
  {
    return (strcmp(rID ,p->m_parmData->recordSaveID) == 0);
  }
  char *rID;

}finder_t;


//参数队列
typedef struct liveParmStruct
{
   char *liveParm;
   char *liveType;
}liveParmStruct;

static pthread_mutex_t list_lock = PTHREAD_MUTEX_INITIALIZER;

/*信号量*/
sem_t bin_sem;
sem_t bin_blank;

std::list<RecordSaveRunnable*> RecordSaveList; //直播对象链表
std::queue<liveParmStruct*> liveParmQueue; //直播参数队列


static const char *s_http_port = "8000";
static const char *cdnUrl = "rtmp://www.bj-mobiletv.com:8000/live/";

//string  LOGFOLDER =  "/home/record/recordlog/";

string  LOGFOLDER =  "./recordlog/";

string  serverName = "录制服务29";

//线程对象
pthread_t record_t;

pthread_t httpServer_t;

pthread_t httpTime_t;

pthread_t checkDisk_t;


static int httpSev_flag = 1; //http服务线程退出标志

static int recordMange_flag = 1; //任务管理线程退出标志

static int record_flag = 1; //录制服务在线状态上传标志

static int record_serverId = 0;  //录制服务ID

static char *response;

//http请求对象
LibcurClient *m_httpclient, *s_httpclient;


//http监听服务线程处理数据
void ev_handler(struct mg_connection *nc, int ev, void *ev_data) 
{
   switch (ev) 
   {
    case MG_EV_ACCEPT: {
      char addr[32];
      mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      LOG(INFO) << "Connection from:"<<nc<<addr;
      break;
    }
    case MG_EV_HTTP_REQUEST: {

      struct http_message *hm = (struct http_message *) ev_data;
      char addr[32];
      mg_sock_addr_to_str(&nc->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      printf("%p: %.*s %.*s %.*s\r\n", nc, (int) hm->method.len, hm->method.p,
             (int) hm->uri.len, hm->uri.p ,(int) hm->body.len, hm->body.p);

      char url[(int)hm->uri.len];
      sprintf(url, "%.*s",(int)hm->uri.len,hm->uri.p);

      printf("get url:%s \n",url);
   
      char urls[11];
      sprintf(urls, "%.*s",11,hm->uri.p);

      int ret = 0;
      if(strcmp(url, "/live/record") == 0)
      {
         
         printf("start: %s\n" , url);
         LOG(INFO) << "开始解析url:"<<url;
         //处理参数
         char parmStr[(int) hm->query_string.len];
         sprintf(parmStr, "%.*s",(int) hm->query_string.len,hm->query_string.p);
          
         char *liveId = (char*)malloc(sizeof(char)*128);
         memset(liveId, 0 ,sizeof(char)*128);
         if(NULL == liveId)
         {       
             LOG(ERROR) << "参数liveId分配内存失败:"<<liveId;
             ret = 1;
             goto end;            
         }

         char *type = (char*)malloc(sizeof(char)*128);
         memset(type, 0 ,sizeof(char)*128);
         if(NULL == type)
         {
              LOG(ERROR) << "参数type分配内存失败:"<<type;
              ret = 2;
              goto end;
         }
         mg_get_http_var(&hm->query_string, "liveId", liveId, 128);//获取liveID
	       mg_get_http_var(&hm->query_string, "type", type, 128);//获取接口类别

         printf("参数2 : %s %s\n",liveId ,type);
         LOG(INFO) << "参数2: "<<liveId<<"  "<<type;   

         //表示目前参数类型只支持0或1
         if((strcmp(type,"0") == 0) || (strcmp(type,"1") == 0))
         {
            if(strlen(liveId) == 0)
            {
               printf("%s\n","liveld is empty");
               LOG(ERROR)<<"Error ,liveld is empty";
               ret = 3;
               goto end;
           }

          liveParmStruct *m_parmData = (liveParmStruct*)malloc(sizeof(liveParmStruct));
          m_parmData->liveType = type;
          m_parmData->liveParm = liveId;

          sem_wait(&bin_blank);
                
          liveParmQueue.push(m_parmData);

          sem_post(&bin_sem);    

        }else
        {
            printf("%s\n","Type is error");
            LOG(ERROR)<<"Error ,Type is error";
            ret = 4;
            goto end;
        }             
     }else
     {
        ret = -1;
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
    
    case MG_EV_CLOSE: {
      printf("%p: Connection closed\r4\n", nc);
      LOG(INFO) << "Connection closed:"<<nc;   
      break;
    }
  }
}


//停止录制任务
void *deleteRecord_fun(void *data)
{
     liveParmStruct *paramdata = (liveParmStruct *)data;

     std::list<RecordSaveRunnable*>::iterator iter ;
	 
     pthread_mutex_lock(&list_lock);
     iter = find_if(RecordSaveList.begin(),RecordSaveList.end(), finder_t(paramdata->liveParm));

     if(iter != RecordSaveList.end())
     {         
          printf("find: %s\n", (*iter)->m_parmData->recordSaveID);
          LOG(INFO) << "find:"<<(*iter)->m_parmData->recordSaveID;    
          int ret = (*iter)->StopRecord();

          if(0 == ret)
          {    
               RecordSaveList.erase(iter);

               RecordSaveRunnable *m_runnable = *iter;
               printf("%s %s\n","删除录制 ID:",m_runnable->m_parmData->recordSaveID);
               LOG(INFO)<< "删除录制 ID:"<<m_runnable->m_parmData->recordSaveID <<"  "<<"ret:"<<ret;
               if(NULL != m_runnable)
               {
                  delete(m_runnable);
                  m_runnable = NULL;
               }              
        
          }else
          {
               LOG(ERROR) << "删除录制任务失败 ID:"<<(*iter)->m_parmData->recordSaveID<<" "<<"ret:"<<ret;
          }            
     }
     else
     {
         LOG(ERROR) << "未找到录制任务:"<<paramdata->liveParm;
     }
     pthread_mutex_unlock(&list_lock);
   
     if(NULL != paramdata->liveType)
     {
           free(paramdata->liveType);
           paramdata->liveType = NULL;
     }

     if(NULL != paramdata->liveParm)
     {
           free(paramdata->liveParm);
           paramdata->liveParm = NULL;
     }
    
     pthread_detach(pthread_self()); 

     return data;
}


//增加录制任务
void *addRecord_fun(void *data)
{    
     liveParmStruct *parmdata = (liveParmStruct*)data;

     LOG(INFO) << "启动任务线程 type:"<<parmdata->liveType<<"  ID:"<<parmdata->liveParm;

     ParmData *pdata =(ParmData*)malloc(sizeof(ParmData));
     if(NULL == pdata)
     {       
          LOG(ERROR) << "ParmData分配内存失败 liveID:"<<parmdata->liveParm;
          return data;           
     }
     memset(pdata, 0 ,sizeof(ParmData));
    
     int len = strlen(parmdata->liveParm); 
     pdata->recordSaveID  = (char*)malloc(sizeof(char)*len + 1);
    
     printf("canshu22: %d\n", len+1);
     if(NULL == pdata->recordSaveID)
     {       
          LOG(ERROR) << "ParmData recordSaveID 分配内存失败 liveID:"<<parmdata->liveParm;
          return data;               
     }
     memset(pdata->recordSaveID, 0 ,len);
     strcpy(pdata->recordSaveID, parmdata->liveParm);
    
     len += strlen(cdnUrl);
     pdata->rtmpUrl = (char*)malloc(sizeof(char)*len + 1);
     if(NULL == pdata->rtmpUrl)
     {       
          LOG(ERROR) << "ParmData rtmpUrl 分配内存失败 liveID:"<<parmdata->liveParm;
          return data;             
     }

     memset(pdata->rtmpUrl, 0 ,len+1);
     sprintf(pdata->rtmpUrl, "%s%s",cdnUrl,parmdata->liveParm);

     printf("jiegouti: %s  %s\n", pdata->recordSaveID , pdata->rtmpUrl);

     RecordSaveRunnable *recordRun = new RecordSaveRunnable(pdata);
    
     int ret = recordRun->CmdRecord();

     if(ret == 0)
     {
          pthread_mutex_lock(&list_lock);

          RecordSaveList.push_back(recordRun);
        
          pthread_mutex_unlock(&list_lock);

          LOG(INFO) << "已启动任务线程 ID: "<<parmdata->liveParm<<"  "<<"ret:"<<ret;
     }else
     { 
     	   LOG(ERROR) << "任务线程启动失败 ID: "<<parmdata->liveParm<<"  "<<"ret:"<<ret;
		   if(NULL!=recordRun)
		   {
			  delete recordRun;
              recordRun = NULL;
		   }       
     }

     if(NULL != parmdata->liveType)
     {
           free(parmdata->liveType);
           parmdata->liveType = NULL;
     }

     if(NULL != parmdata->liveParm)
     {
           free(parmdata->liveParm);
           parmdata->liveParm = NULL;
     }
     
     pthread_detach(pthread_self()); 

     return data;

}
//录制任务对象管理线程
void *recordManage_fun(void *data)
{
    printf("recordManage_fun :[tid: %ld]\n", syscall(SYS_gettid));
    
    int ret = 0;
    while(recordMange_flag)
    {      
         
         // struct timespec outtime;
         // struct timeval now;

         // gettimeofday(&now, NULL);

         // outtime.tv_sec = now.tv_sec;
         // outtime.tv_nsec = now.tv_usec*1000 + 10 * 1000 * 1000;
         // outtime.tv_sec += outtime.tv_nsec/(1000 * 1000 *1000);
         // outtime.tv_nsec %= (1000 * 1000 *1000);

         // ret = sem_timedwait(&bin_sem ,&outtime); 

         // if(ret == -1)
         // {
         //    if(httpSev_flag == 0)
         //    {
         //        recordMange_flag= 0;
         //        break;
         //    }else
         //    {
         //        continue;
         //    }       
         // }
        sem_wait(&bin_sem); 

        liveParmStruct *pdata = liveParmQueue.front();
                
        LOG(INFO) << "管理线程获取参数 type:"<<pdata->liveType<<"  ID:"<<pdata->liveParm;

        if(strcmp(pdata->liveType, "0") == 0) //开始
        {   
            pthread_t addlistlist_t;
            ret = pthread_create(&addlistlist_t, NULL, addRecord_fun, (void *)pdata);
            if(ret!= 0)
            {
            	
            	LOG(ERROR) << "创建增加线程失败 ID:"<<pdata->liveParm<<" "<<"ret:"<<ret;
            }
       
        }else if(strcmp(pdata->liveType, "1") == 0)  //结束
        {
            
            pthread_t deletelist_t;
            ret = pthread_create(&deletelist_t, NULL, deleteRecord_fun, (void *)pdata);

            if(ret!=0)
            {
            	
               LOG(ERROR) << "创建结束线程失败 ID: "<<pdata->liveParm<<" "<<"ret:"<<ret;
            }
                
        }

        liveParmQueue.pop();
       
        sem_post(&bin_blank);  
    }

    return data;
}


//http服务监听线程
void *httpServer_fun(void *pdata)
{
   printf("httpServer_fun :[tid: %ld]\n", syscall(SYS_gettid));
   struct mg_mgr mgr;
   struct mg_connection *nc;

   mg_mgr_init(&mgr, NULL);
   printf("Starting web server on port %s\n", s_http_port);
   LOG(INFO) << "http服务监听线程 Starting web server on port"<<s_http_port;
   nc = mg_bind(&mgr, s_http_port, ev_handler);
   if (nc == NULL) 
   {
      printf("Failed to create listener\n");
      LOG(ERROR) << "http服务监听线程 Failed to create listener";
      httpSev_flag = 0;
      pthread_detach(pthread_self());
      return 0;
   }

   mg_set_protocol_http_websocket(nc);

   while(httpSev_flag)
   {    
      mg_mgr_poll(&mgr, 1000);
   }

   return pdata;
 }

static void checkdisk_fun(int val)
{
    char path[1024] ;
 
    //获取当前的工作目录，注意：长度必须大于工作目录的长度加一
    char *p = getcwd(path , 1024);
    printf("buffer:%s  p:%s size:%zu\n" , path , p , strlen(path));

    struct statfs diskInfo;
    statfs(path, &diskInfo);
    unsigned long long totalBlocks = diskInfo.f_bsize;
    unsigned long long totalSize = totalBlocks * diskInfo.f_blocks;
    size_t mbTotalsize = totalSize>>20;
    unsigned long long freeDisk = diskInfo.f_bfree*totalBlocks;
    size_t mbFreedisk = freeDisk>>20;
    printf ("%s: total=%lld total=%zuMB,free=%lld,  free=%zuMB\n",path, totalSize , mbTotalsize, freeDisk ,mbFreedisk);

    if(mbFreedisk < 2048)
    {
       printf("Waring:%zu disk will full!!!! \n",mbFreedisk);
       LOG(WARNING)<<"Disk will full freedisk:"<<mbFreedisk;
    }
}

void seconds_sleep(unsigned seconds)
{
    struct timeval tv;
    time_t tt;
    //char* p=NULL;
    tv.tv_sec=seconds;
    tv.tv_usec=0;
    int err;
 do{
     err=select(0,NULL,NULL,NULL,&tv);
     time(&tt);
     //p=ctime(&tt);
     //printf("%s\n",p);
     checkdisk_fun(20);
  }while(err<0 && errno==EINTR);
}


//定时上传状态线程
void *httpTime_fun(void *pdata)
{
   s_httpclient = new LibcurClient;

   string url = IPPORT;
   url = url.append("server_update?serverId=");
   char numStr[1024] ={};
   snprintf(numStr, sizeof(numStr), "%d",record_serverId);
   url.append(numStr);
   url.append("&netFlag=20");
   cout<<"time url:"<<url<<endl;

   //录制服务在线状态定时上传
   while(record_flag)
   {
      sleep(10);

      int main_ret = s_httpclient->HttpGetData(url.c_str());
      if(main_ret != 0)
      {
         LOG(ERROR) << "定时上传录制服务状态失败 "<<"错误代号:"<<main_ret;  

      }else
      {
         std::string res = s_httpclient->GetResdata();
         printf("888:%s\n", res.c_str());

        if(!(s_httpclient->GetResdata()).empty())
        {
           json m_object = json::parse(s_httpclient->GetResdata());
           if(m_object.is_object())
           {
               string resCode = m_object.value("code", "oops");
               main_ret = atoi(resCode.c_str() );

               if(0 != main_ret)
               {
                  std::cout<<main_ret<<endl;
                  LOG(ERROR) << "定时上传录制服务状态失败 main_ret:"<< main_ret <<" "<<"错误信息:"<<m_object.at("msg");   
               }
            }
       }else
       {
       	  LOG(ERROR) << "获取定时上传录制服务数据为空!";
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

//定时检测磁盘
void *checkDisk_fun(void *data)
{
   while(record_flag)
   {  
      seconds_sleep(60 * 30);
   }

   return data;
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
                      return -1;   
                  }                   
              }  
              DirName[i] = '/';  
          }  
      }  
      return 0;  
} 

//start server
int startServer(void)
{

    int main_ret = 0;
	  std::string resStr = "";
    m_httpclient = new LibcurClient;
    std::string url;

    char path[1024] ;
    //获取当前的工作目录，注意：长度必须大于工作目录的长度加一
    char *p = getcwd(path , 1024);

    printf("flod :%s\n", p);

	  main_ret = CreateLogFileDir(LOGFOLDER.c_str());
	  if(0 != main_ret)
	  {
       LOG(ERROR) << "创建log 文件夹失败"<<" "<<"main_ret:"<<main_ret;
       return main_ret ;
	  }

	//创建log初始化
    google::InitGoogleLogging("");

    string logpath = LOGFOLDER + "recordServer-";
    google::SetLogDestination(google::GLOG_INFO,logpath.c_str());
    FLAGS_logbufsecs = 0; //缓冲日志输出，默认为30秒，此处改为立即输出
    FLAGS_max_log_size = 500; //最大日志大小为 100MB
    FLAGS_stop_logging_if_full_disk = true; //当磁盘被写满时，停止日志输出


    //信号量初始化
    main_ret = sem_init(&bin_sem, 0, 0);
    if(0 != main_ret)
    {
       
      LOG(ERROR) << "bin_sem创建失败"<<" "<<"main_ret:"<<main_ret;
      return main_ret ;
    }
 
    main_ret = sem_init(&bin_blank, 0, 800);
    if(0 != main_ret)
    {
      
      LOG(ERROR) << "bin_blank创建失败"<<" "<<"main_ret:"<<main_ret;
      return main_ret ;
    }

    //获取cdn有关数据http接口 
    url = IPPORT;
    url = url.append("cdn_select?cdnId=1"); 
    main_ret = m_httpclient->HttpGetData(url.c_str());
    if(main_ret != 0)
    {

        LOG(ERROR) << "获取CDN数据失败: "<<resStr<<" "<<"错误代号:"<<main_ret;  
        return main_ret;

    }else
    {

       if(!(m_httpclient->GetResdata()).empty())
       {
       	   json m_object = json::parse(m_httpclient->GetResdata());
           if(m_object.is_object())
           {
               string resCode = m_object.value("code", "oops");
               main_ret = atoi(resCode.c_str() );
               if(0 != main_ret)
               {
                  std::cout<<main_ret<<endl;
                  LOG(ERROR) << "获取CDN数据失败 main_ret:"<< main_ret <<" "<<"错误信息:"<<m_object.at("msg");
                  return main_ret;
               }else
               {
               }
           }
       }else
       {
       	    LOG(ERROR) << "获取CDN数据为空!";
       	    return main_ret;
       }
   }
        
    //注册录制服务接口  
    // url = IPPORT;
    // url.append("server_create?serverType=2&serverName=");
    
    // char *format = m_httpclient->UrlEncode(serverName);
    // url.append(format);
    // url.append("&netFlag=1&serverIp=192.168.1.206:8000"); 

    // printf("%s\n", url.c_str());

    // main_ret = m_httpclient->HttpGetData(url.c_str());
    // if(main_ret != 0)
    // {

    //    cout<<main_ret<<endl;
    //    LOG(ERROR) << "注册录制服务失败  "<<"错误代号:"<<main_ret;  
    //    return main_ret;

    // }else
    // {
    //       std::string res = m_httpclient->GetResdata();
    //       printf("777:%s\n", res.c_str());

    //       if(!(m_httpclient->GetResdata()).empty())
    //       {
    //         json m_object = json::parse(m_httpclient->GetResdata());
    //         if(m_object.is_object())
    //         {
    //            string resCode = m_object.value("code", "oops");
    //            main_ret = atoi(resCode.c_str() );

    //            if(0 != main_ret)
    //            {
    //              std::cout<<main_ret<<endl;
    //              LOG(ERROR)<<"注册录制服务失败 main_ret:"<< main_ret <<" "<<"错误信息:"<<m_object.at("msg");
    //              return main_ret ;
    //            }else
    //            {
    //              //std::cout << m_object.at("code") << '\n';
    //              record_serverId = m_object.value("serverId", 0);
    //              printf("serverId :%d\n", record_serverId);

    //            }
    //        }
    //     }else
    //     { 
    //    	    LOG(ERROR) << "获取注册录制服务数据为空!";
    //    	    return main_ret;
    //     }
    // }

    //往消息队列里面写数据
    int msqid = getMsgQueue();
    char numStr[1024] ={0};
    snprintf(numStr, sizeof(numStr), "%d",record_serverId);
    sendMsg(msqid, CLIENT_TYPE, numStr);
     
    //创建录制任务管理线程
    main_ret = pthread_create(&record_t, NULL, recordManage_fun, NULL);
    if(0 != main_ret)
    {
     
     LOG(ERROR) << "录制管理线程创建失败"<<" "<<"main_ret:"<<main_ret; 
     return main_ret; 
    }

    //创建http服务线程  
    main_ret = pthread_create(&httpServer_t, NULL, httpServer_fun, NULL);
    if(0 != main_ret)
    {
    
      LOG(ERROR) << "http服务监听线程创建失败"<<" "<<"main_ret:"<<main_ret; 
      return main_ret;   
    }

    //创建定时上传线程
    main_ret = pthread_create(&httpTime_t,NULL, httpTime_fun, NULL);
    if(0 != main_ret)
    {
     LOG(ERROR) << "http定时上传线程创建失败"<<" "<<"main_ret:"<<main_ret; 
     return main_ret;   
    }

    //创建定时检测磁盘线程
    main_ret = pthread_create(&checkDisk_t,NULL, checkDisk_fun, NULL);
    if(0 != main_ret)
    { 
     LOG(ERROR) << "定时检测磁盘线程创建失败"<<" "<<"main_ret:"<<main_ret; 
     return main_ret;   
    }

    return  main_ret;
}


//stop server
int stopServer(void)
{   
 
    int main_ret = 0;

//     char stopStr[1024] = {0};

//     std::string resStr = "";

// here:
//    scanf("%s",stopStr);

//    if(strcmp("stop",stopStr) == 0)
//    {
//        pthread_cancel(checkDisk_t);//取消线程
//        pthread_cancel(httpTime_t);//取消线程
//    }else
//    {   
//        printf("请输入正确的停止命令!\n");
//        memset(stopStr, 0 ,1024);
//        goto here;
//    }

   //注册录制服务离线接口
   // main_ret = m_httpclient->HttpGetData("http://192.168.1.205:8080/live/server_create?serverType=4&serverName=serverqw&netFlag=0&serverIp=192.168.1.206:8000");
   // if(main_ret != 0)
   // {

   //     LOG(ERROR) << "注册录制服务离线失败: "<<resStr<<" "<<"错误代号:"<<main_ret;  
   //     return main_ret;

   // }else
   // {
   //    json m_object = json::parse(m_httpclient->GetResdata());
   //    if(m_object.is_object())
   //    {
   //       string resCode = m_object.value("code", "oops");
   //       main_ret = atoi(resCode.c_str() );

   //       if(0 != main_ret)
   //       {
   //           std::cout<<main_ret<<endl;
   //           LOG(ERROR) << "注册录制服务离线失败 main_ret:"<< main_ret <<" "<<"错误信息:"<<m_object.at("msg");
   //           return main_ret ;
   //       }
   //    }
   // }
  
    //定时上传录制状态线程退出
    //record_flag = 0; 
    main_ret = pthread_join(httpTime_t,NULL);
    if(0 != main_ret)
    {
       LOG(ERROR) << "定时上传录制状态线程退出错误"<<" "<<"main_ret:"<<main_ret;
       return main_ret;
    }

    //定时检测磁盘线程退出
    main_ret = pthread_join(checkDisk_t,NULL);
    if(0 != main_ret)
    {
       LOG(ERROR) << "定时检测磁盘线程退出错误"<<" "<<"main_ret:"<<main_ret;
       return main_ret;
    }

    //等待http服务线程退出
    //httpSev_flag = 0; 
    main_ret = pthread_join(httpServer_t, NULL);
    if(0 != main_ret)
    {
     
       LOG(ERROR) << "http服务线程退出错误"<<" "<<"main_ret:"<<main_ret;
       return main_ret;  
    } 
    printf("%s %d\n", "http服务线程退出",main_ret);
    LOG(INFO) << "http服务线程退出: "<<main_ret;


    //等待录制管理线程退出
    //recordMange_flag = 0; 
    main_ret = pthread_join(record_t,NULL);
    if(0 != main_ret)
    {
       LOG(ERROR) << "录制管理线程退出错误"<<" "<<"main_ret:"<<main_ret;
       return main_ret;
    }

    printf("%s %d\n", "录制管理线程退出",main_ret);
    LOG(INFO) << "录制管理线程退出: "<<main_ret;

    /*销毁互斥*/
    main_ret = sem_destroy(&bin_sem);
    if(0 != main_ret)
    {
       LOG(ERROR) << "销毁互斥bin_sem错误"<<" "<<"main_ret:"<<main_ret;
       return main_ret;
    }		

    main_ret = sem_destroy(&bin_blank);
    if(0 != main_ret)
    {
       LOG(ERROR) << "销毁互斥bin_blank错误"<<" "<<"main_ret:"<<main_ret;
       return main_ret;
    }

    if(NULL != m_httpclient)
    {
      delete m_httpclient;
      m_httpclient = NULL;
    }
 
    if(NULL != response)
    {
       free(response);
       response = NULL;
    }

    return main_ret;
}


int main(int argc, char* argv[]) 
{
   int main_ret = 0;

   printf("record_Server :[tid: %ld]\n", syscall(SYS_gettid));

   //启动录制服务
   main_ret = startServer();

   if(0 != main_ret)
   {
     LOG(INFO) << "server start error:  "<<"main_ret:"<<main_ret;
     return main_ret;
   }

   printf("server starting\n");
   LOG(INFO) << "server start: "<<"main_ret:"<<main_ret;

   //定时检测磁盘空间
   // while(record_flag)
   // {  
   //    seconds_sleep(60 * 30);
   // }

   //停止录制服务
   main_ret = stopServer();
   if(0 == main_ret)
   {
        LOG(INFO) << "server stop 正常退出"<<" "<<"main_ret:"<<main_ret;
   }else
   {
       LOG(ERROR) << "server stop 异常退出"<<" "<<"main_ret:"<<main_ret;
       printf("server stop ret:%d\n", main_ret);
   }

   return main_ret;
}
