#include "RecordSaveRunnable.h"
#include "glog/logging.h"

RecordSaveRunnable::RecordSaveRunnable(ParmData *pdata)
{

     m_parmData = pdata;  //传结构体指针

     recive_http = new LibcurClient;

     save_http = new LibcurClient;

     save_httpflag = true;

     recive_httpflag = 0; //录制状态

     iHasAudioSpecificConfig = 0;
     ah = {0};
     ad = {0}; 

     rtmpQueue = InitQueue();
    
     sem_init(&bin_sem, 0, 0);
     sem_init(&bin_blank, 0, MaxSize);

     runningp = 1;  
     runningc = 1;   

     firstflag = true;  

     lRecvBytes= 0;

     Iread = 0;
     Iwrite = 0;
     tagI = 0;

     m_ret = 0;

     aacTagNum = 0; //aacTag计数器


    Aaacfirst_timeStamp = 0;
    H264first_timeStamp = 0;

    AaacEnd_timeStamp = 0;
    H264End_timeStamp = 0;

    m_AacFrist = true;
    m_H264Frist = true;

    m_writeAaacH264 = true;
}

//解析http返回json
int RecordSaveRunnable::ParseJsonInfo(std::string &jsonStr ,std::string &resCodeInfo ,std::string &liveinfo)
{
    int main_ret = 0;

    if(!jsonStr.empty())
    {  
        json m_object = json::parse(jsonStr);
  
       if(m_object.is_object())
       {
         string resCode = m_object.value("code", "oops");
         main_ret = atoi(resCode.c_str() );

         if(0 != main_ret)
         {
             std::cout<<main_ret<<endl;

             LOG(ERROR)<<" 返回http接口失败!";
             return main_ret;
         }else
         {      
            resCodeInfo = m_object.value("msg", "oops");
          
            //LOG(INFO)<<"返回http接口信息  msg:"<<resCodeInfo;
            auto it_liveinfo = m_object.find("live_info");
            if(it_liveinfo != m_object.end())
            {
                json liveinfoObj = m_object.at("live_info");
                if(liveinfoObj.is_object())
                {
                    liveinfo = liveinfoObj.value("liveFlag", "oops");
                    LOG(INFO)<<"返回http 直播状态  liveFlag:"<<liveinfo;
                    //std::cout<<liveinfo<<endl;
                }else
                {
                    liveinfo = "3";
                } 
            }else
            {
               liveinfo = "3";
            }   
            return main_ret;
         }
        }else
        {
           LOG(ERROR)<<" 返回http 接口数据不全!";
           main_ret = 1;
           return main_ret;
        }
    }else
    {
        LOG(ERROR) << "获取 RecordSaveRunnable::ParseJsonInfo 数据为空!";
        int main_ret = 2;
        return main_ret;
    }
}

//启动录制线程
int RecordSaveRunnable::CmdRecord()
{

      int ret = CreateFile();  //新建文件
      if(0 != ret)
      {
          LOG(ERROR) << "新建文件失败:"<<ret<<" "<<m_parmData->recordSaveID;
          return ret;
      }

      ret = pthread_create(&producter_t, NULL, Recive_fun, (void *)this); //&m_ret
      if(0 != ret)
      {
         LOG(ERROR) << "创建接收线程失败:"<<ret<<" "<<m_parmData->recordSaveID;
         return ret;
      }

      ret =  pthread_create(&consumer_t, NULL, Save_fun,  (void *)this);
      if(0 != ret)
      {
         LOG(ERROR) << "创建保存线程失败: "<<ret<<" "<<m_parmData->recordSaveID;
         return ret;
      }

      printf("已启动录制和保存线程 rtmp: %s \n",m_parmData->rtmpUrl);
      LOG(INFO) << "已启动录制和保存线程 rtmp: "<<m_parmData->rtmpUrl;
       
      return ret;
} 

int RecordSaveRunnable::CreateFileDir(const char *sPathName)  
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
                  printf("mkdir file  error\n");
                  return -1;   
              }  
                 
            }  
            DirName[i] = '/';  
       }  
    }  
    return 0;  
} 

int RecordSaveRunnable::CreateFile()
{
  
  int ret = 0;
  int i = 0;
  char aFileStr[1024] = {0};
  char vFileStr[1024] = {0};
  char wFileStr[1024] ={0};
 

  //file dir
  char fileDir[1024] = {0};  
 
  sprintf(fileDir ,"%s%s%s",FILEFOLDER, m_parmData->recordSaveID,"/");

  ret = CreateFileDir(fileDir);
  if(0 != ret)
  {
    LOG(ERROR)<<" 创建file 文件夹失败!";
    return ret;   
  }
  
  printf("fileDir: %s\n", fileDir);

  sprintf(aFileStr ,"%s%s%s",fileDir, m_parmData->recordSaveID, ".aac");

  sprintf(vFileStr ,"%s%s%s",fileDir, m_parmData->recordSaveID, ".h264");

  sprintf(wFileStr ,"%s%s%s",fileDir, m_parmData->recordSaveID, ".json");
  
  while((access(aFileStr, F_OK)) != -1)
  {
     i++;
     memset(aFileStr,0,1024);
     sprintf(aFileStr,"%s%s%s%d%s",fileDir,m_parmData->recordSaveID,"(+",i,").aac");

     memset(vFileStr,0,1024);
     sprintf(vFileStr,"%s%s%s%d%s",fileDir,m_parmData->recordSaveID,"(+",i,").h264");

     memset(wFileStr,0,1024);
     sprintf(wFileStr,"%s%s%s%d%s",fileDir,m_parmData->recordSaveID,"(+",i,").json");
  }

  printf("文件名: %s %s %s \n", aFileStr, vFileStr, wFileStr);
  LOG(INFO) << "文件名: "<<aFileStr<<"  "<<vFileStr<<"  "<<wFileStr;

  afile = fopen(aFileStr,"ab+");
  if(NULL == afile)
  {
      ret = errno;
      LOG(ERROR)<<" 打开aac文件失败 ID:"<<m_parmData->recordSaveID<< "  ret"<<ret<<"  reason:"<<strerror(ret);
      return ret;
  }

  vfile = fopen(vFileStr,"ab+");
  if(NULL == vfile)
  {
     ret = errno;
     LOG(ERROR)<<" 打开h264文件失败 ID:"<<m_parmData->recordSaveID<< "  ret"<<ret<<"  reason:"<<strerror(ret);
     return ret;

  }

  wfile = fopen(wFileStr,"ab+");
  if(NULL == wfile)
  {
     ret = errno;
     LOG(ERROR)<<" 打开json文件失败 ID:"<<m_parmData->recordSaveID<< "  ret"<<ret<<"  reason:"<<strerror(ret);
     return ret;
  }

  return ret;

}
 int RecordSaveRunnable::StopRecord()
 {

      runningp = 0;

      int resCode = pthread_join(producter_t,NULL);

      if(0 != resCode)
      {
          LOG(ERROR) << "销毁接收线程失败: "<<resCode<<" "<<m_parmData->recordSaveID;
          return resCode;
      }

      resCode = pthread_join(consumer_t,NULL);

      if(0 != resCode)
      {
          LOG(ERROR) << "销毁保存线程失败: "<<resCode<<" "<<m_parmData->recordSaveID;
          return resCode;
      }

       /*销毁互斥*/
      resCode = sem_destroy(&bin_sem);
      if(0 != resCode)
      {
          LOG(ERROR) << "销毁互斥bin_sem失败: "<<resCode<<" "<<m_parmData->recordSaveID;
          return resCode;
      }
     
      resCode = sem_destroy(&bin_blank); 
      if(0 != resCode)
      {
          LOG(ERROR) << "销毁互斥bin_blank失败: "<<resCode<<" "<<m_parmData->recordSaveID;
          return resCode;
      }
   
      return resCode;
}


//生产者线程静态函数
void  *RecordSaveRunnable::Recive_fun(void* arg)
{

    return static_cast<RecordSaveRunnable*>(arg)->rtmpRecive_f();

}

//消费者线程静态函数
void *RecordSaveRunnable::Save_fun(void *arg)
{
   
    return static_cast<RecordSaveRunnable*>(arg)->rtmpSave_f();
}


void *RecordSaveRunnable::rtmpRecive_f()
{
    LOG(INFO) << "开始解析 rtmp: "<<m_parmData->rtmpUrl;

    std::string resCodeInfo ,liveinfo ,urlparm;

    int bufsize = 1024 * 1024 * 10; 
    buf = (char*)malloc(sizeof(char) * bufsize);
    memset(buf, 0, bufsize);

    if(NULL == buf)
    {
       LOG(ERROR) << "rtmpInit no free memory!!!";
       return  (void*)0;
    }

    double duration = 0.0;
    uint32_t bufferTime = (uint32_t) (duration * 1000.0) + 5000; 

    int re_Connects = 0;  //断开重连次数

    int re_Read = 0; //读取sokcet数据超时次数
 
begin: 

     m_pRtmp = RTMP_Alloc();
    
     RTMP_Init(m_pRtmp);

     m_pRtmp->Link.timeout = 30;
    
     printf("开始解析 222rtmp: %s \n",m_parmData->rtmpUrl);
     LOG(INFO) << "开始解析222  rtmp:"<<m_parmData->rtmpUrl;

     if(!RTMP_SetupURL(m_pRtmp,(char*)TESTURL))   //m_parmData->rtmpUrl
     {
         LOG(ERROR) << "拉流地址设置失败！ "<<m_parmData->rtmpUrl;    
         m_ret = 3;
         goto end;
     }
     
    m_pRtmp->Link.lFlags |= RTMP_LF_LIVE; 
    RTMP_SetBufferMS(m_pRtmp, bufferTime);

     if(!RTMP_Connect(m_pRtmp, NULL))
     {
          LOG(ERROR) << "RTMP服务连接失败！ ";	    
          m_ret = 4;
          goto end;
     }
   
    if(!RTMP_ConnectStream(m_pRtmp, 0))
    {	
         LOG(ERROR) << "拉流连接失败！ "<<m_parmData->rtmpUrl;
         m_ret = 5;
         goto end;
    }
    
    LOG(INFO) << "录制开始！ "<<m_parmData->rtmpUrl;

    while(runningp) 
    {    
        int nRead = RTMP_Read(m_pRtmp, buf, bufsize);	
       
        if(nRead > 0)
        {     
           if(0 != re_Connects)
           {
             firstflag = true;  
           	 re_Connects = 0;
           }

           if(0 != re_Read)
           {
              re_Read = 0;
           }
           
           sem_wait(&bin_blank);

           tagI++;

           //printf("直播ID: %s 生产者:tagID wei %d  %d\n:", m_parmData->recordSaveID ,tagI , nRead);
           //LOG(INFO) << "直播ID: "<<m_parmData->recordSaveID<<" 生产者:"<<tagI<<"  "<<nRead;

           RtmpRStruct rtmpNode;
           char *s = buf;
           rtmpNode.prtmpdata = (char*)malloc(nRead);
           if(NULL == rtmpNode.prtmpdata)
           {
              LOG(ERROR) << "rtmpNode申请内存失败!";
              continue;
           }

           memcpy(rtmpNode.prtmpdata ,s ,nRead);
           rtmpNode.mread = nRead;

           EnQueue(rtmpQueue,rtmpNode);
           sem_post(&bin_sem);
        
           lRecvBytes += nRead;

        }else
        {   
              //判断rtmp连接是否中断
              if(!RTMP_IsConnected(m_pRtmp))
              { 

                if(0 != re_Read)
                {
                	re_Read = 0;
                }
                //连接中断,调用修改直播接口,禁止写线程调用
                RTMP_Log(RTMP_LOGDEBUG, "RTMP连接断开！!");

                re_Connects++;

                if(re_Connects < 3)
                {
                	  printf("RTMP连接准备重连 ,直播过程中 直播ID:%s\n" ,m_parmData->recordSaveID);

                    LOG(INFO) << "直播过程中RTMP连接准备重连 直播ID:"<<m_parmData->recordSaveID;
                     
                    RTMP_Close(m_pRtmp);
                  
                    if(NULL != m_pRtmp)
                    {
                       RTMP_Free(m_pRtmp);
                       m_pRtmp = NULL;
                    }
                    goto begin;
                }  

                save_httpflag = false; 
                runningp = 0; 
                recive_httpflag = 5; //直播标志设置为1

                //RTMP连接断开,调用修改直播接口,禁止写线程调用   
                
                m_ret = UpdataRecordflag(recive_http,recive_httpflag); 
                break;

              }else
              {
                    if(RTMP_IsTimedout(m_pRtmp))
                    {
                          
                       //直播查询,看直播是否中断 
                       urlparm = "live_select?liveId=";
                       urlparm.append(m_parmData->recordSaveID);
                       string urlStr = IPPORT + urlparm;

                       m_ret = recive_http->HttpGetData(urlStr.c_str());
                       if(0 != m_ret)
                       {
                            cout<<m_ret<<endl;
                            LOG(ERROR) << "直播查询失败 "<<"错误代号:"<<m_ret;
                            re_Read++;
                         
                            //读取sokcet数据超时
                            if(re_Read > 1)
                            {
                               runningp = 0;
                               recive_httpflag = 7; //直播标志设置为7,                           
                               LOG(ERROR) << "直播过程中读取sokcet数据超时 直播ID:"<<m_parmData->recordSaveID;

                               //能连上rtmp，但一直读不到数据,调用修改直播接口,禁止写线程调用
                               save_httpflag = false;
                               m_ret = UpdataRecordflag(recive_http,recive_httpflag);
                               break; 
                            }
                       }else
                       {
                         std::string  resData = recive_http->GetResdata();
                         m_ret = ParseJsonInfo(resData,resCodeInfo,liveinfo);
                         LOG(INFO) <<"查询直播 直播ID:"<<m_parmData->recordSaveID<<"  直播状态:  "<<liveinfo<< "  ret:"<<m_ret;
                         if("1" == liveinfo)  //查询到还在直播中 rtmp准备重连
                         {
                            re_Connects = 0;
                            RTMP_Close(m_pRtmp);
                            if(NULL != m_pRtmp)
                            {
                                RTMP_Free(m_pRtmp);
                                m_pRtmp = NULL;
                            }
                            LOG(ERROR) << "查询到还在直播中 准备重连 直播ID:"<<m_parmData->recordSaveID;
                            goto begin;

                         }else //查询到直播停止或直播中断
                         {    
                              save_httpflag = false;    
                              runningp = 0;
                              recive_httpflag = atoi(liveinfo.c_str() ); //直播标志设置为客户端异常状态 3或4
                              LOG(ERROR) <<"查询到直播停止或直播中断 直播ID:"<<m_parmData->recordSaveID;
                              
                              //直播停止或直播中断,调用修改直播接口,禁止写线程调用
                              m_ret = UpdataRecordflag(recive_http,recive_httpflag);
                              break;
                         }
                         
                      }
                         
                    }	    
              }
      }
    }
    
end:

	if(RTMP_IsConnected(m_pRtmp))
	{
		 RTMP_Close(m_pRtmp);
	}

	if (NULL != m_pRtmp)
	{
		 RTMP_Free(m_pRtmp);
		 m_pRtmp = NULL;
	}
	
	if(runningp)
	{
		  //printf("RTMP连接准备重连！\n");
      LOG(INFO) << "RTMP连接准备重连!";
		  goto begin;
	}

	if (NULL != buf)
	{
		free(buf);
		buf = NULL;
	}
       if(recive_httpflag == 1 || recive_httpflag == 2)
       {      
          //录制线程收到录制停止命令正常结束
          LOG(INFO) << "录制线程收到命令正常结束!";

       }else if(recive_httpflag == 5)
       {
        
           LOG(ERROR) << "录制异常结束,RTMP连接断开结束!";

       }else
       {        
          //直播停止或直播中断         
          LOG(ERROR) << "录制异常结束,直播停止或中断!";
       }

       LOG(INFO) << "录制线程结束 直播ID: "<<m_parmData->recordSaveID<<"  生产者生产总数目:"<<tagI;

       return  (void*)0;
}

void *RecordSaveRunnable::rtmpSave_f()
{
    struct timespec outtime;
    struct timeval now;
    gettimeofday(&now, NULL);
    outtime.tv_sec = now.tv_sec + 3; //5
    outtime.tv_nsec = now.tv_usec * 1000;

    int value = 0;
    RtmpRStruct rtmWrite;
    int ret = 0;

    while(runningc)
    {
         struct timespec outtime;
         struct timeval now;

         gettimeofday(&now, NULL);

         outtime.tv_sec = now.tv_sec;
         outtime.tv_nsec = now.tv_usec*1000 + 3 * 1000 * 1000;
         outtime.tv_sec += outtime.tv_nsec/(1000 * 1000 *1000);
         outtime.tv_nsec %= (1000 * 1000 *1000);

         ret = sem_timedwait(&bin_sem ,&outtime);  

         if(ret == -1)
         {
            if(runningp == 0)
            {
                //录制正常结束,调用修改直播接口,状态已完成
                if(save_httpflag)
                {
                   recive_httpflag = 2;
                   ret = UpdataRecordflag(save_http,recive_httpflag);
                }

                runningc= 0;
                break;
            }else
            {
                continue;
            }       
         }
       
         DeQueue(rtmpQueue,&rtmWrite);
 
         Iwrite++;
        
         WriteFile(rtmWrite.prtmpdata, rtmWrite.mread ,firstflag);

        
         if(NULL != rtmWrite.prtmpdata)
         {
             free(rtmWrite.prtmpdata);
             rtmWrite.prtmpdata = NULL;
         }
         sem_post(&bin_blank);  

    }

    LOG(INFO) << "保存线程结束 直播ID: "<<m_parmData->recordSaveID<<" 消费者消耗总数目:"<<Iwrite; 

    //不是正常结束 ,删除队列里面录制任务
    if(2 != recive_httpflag)
    {
    	 
         //录制异常结束,删除录制任务
    	   LibcurClient  m_httpclient;
         std::string resStr = "";
         std::string getParm = "http://localhost:8000/live/";
         getParm = getParm.append("record?liveId=");
         getParm.append(m_parmData->recordSaveID);
         getParm.append("&type=1");

         printf("delete url: %s\n", getParm.c_str());
  
         m_httpclient.HttpGetData(getParm.c_str());
    }

    return  (void*)0;    
}

//更新录制状态
int RecordSaveRunnable::UpdataRecordflag(LibcurClient *http_client ,int flag)
{   

    std::string  resCodeInfo ,liveinfo;      
    std::string urlparm = "live_update?liveId=";
   
    urlparm.append(m_parmData->recordSaveID);
    urlparm.append("&recordFlag=");
          
    char flagStr[10] ={};
    snprintf(flagStr, sizeof(flagStr), "%d",flag);
    urlparm.append(flagStr);
    urlparm.append("&operateId=8888");
  
    std::string updataUrl = IPPORT + urlparm;
    //LOG(INFO)<<"UpdataRecordUrl:"<<updataUrl;

    m_ret = http_client->HttpGetData(updataUrl.c_str());

    std::string resData = http_client->GetResdata();
    m_ret = ParseJsonInfo(resData,resCodeInfo ,liveinfo);

    return m_ret;
}

//白板数据写入json文件
bool RecordSaveRunnable::ExtractDefine(char *timebuff, char *data, int len,  int timestamp)
{

   bool bResult = true;
   int iLenVale = 0;
   std::string strDefine;
   if (0x02 != *data || 0x05 != *(data + 2) || 0 != memcmp(data + 3, "onUDD", 5) || 0x08 != *(data + 8))
   {
     printf("%s\n","fail");
     bResult = false;
     return bResult;
   }
   int iArrayNum = 0;
   memcpy(&iArrayNum, data + 9, 4);

   iArrayNum = HTON32(iArrayNum);
   char* pTmp = data + 13;
   for(int i = 0; i < iArrayNum; i++)
   {
    int iLenKey = 0;
    memcpy(&iLenKey, pTmp, 2);
    iLenKey = HTON16(iLenKey);
    pTmp += 2;
    pTmp += iLenKey;
    if (0x02 == *pTmp)//数据类型
    {
      pTmp++;
      //int iLenVale = 0;
      memcpy(&iLenVale, pTmp, 2);
      iLenVale = HTON16(iLenVale);
      pTmp += 2;
      strDefine.append((char*)pTmp, iLenVale);
    }
  }

  printf("shuju: %s  %d  直播ID:%s\n", strDefine.c_str() ,iLenVale ,m_parmData->recordSaveID);

  if(iLenVale != fwrite(strDefine.c_str(), 1, iLenVale, wfile))
  {
     bResult = false;
  }

  return bResult;
}


//aac数据写入aac文件
int RecordSaveRunnable::WriteAac(char *timebuff ,char *data, int len)
{       
	if (data[1] == 0x00)
	{
		iHasAudioSpecificConfig = 1;
		memcpy(&ah, data+2, sizeof(AdtsHeader));
		ad.check1 = 0xff;
		ad.check2 = 0xf; //0xff
		ad.check3 = 0x1f; //0xff
		ad.check4 = 0x3f; //0xff
		ad.protection = 1;
		ad.ObjectType = 0;
		ad.SamplingIndex = ah.SamplIndex2 | ah.SamplIndex1 << 1;
		ad.channel2 = ah.channel;
	}
	else
	{
                
		if(iHasAudioSpecificConfig)
		{
			unsigned int size = len - 2 + 7;
			ad.length1 = (size >> 11) & 0x03;
			ad.length2 = (size >> 3) & 0xff;
			ad.length3 = size & 0x07;

     
			if (sizeof(AdtsData) != fwrite((char*)&ad, 1, sizeof(AdtsData), afile))
			{      
			 	return 1;
		    }
		}
     
    //写入时间戳
    // if(4 != fwrite(timebuff, 1, 4, afile))
    // {
    //  return 2;
    // }

     if(len - 2 != fwrite(data + 2, 1, len - 2, afile))
	   {    
	  	 return 3;
	   }
	}

  aacTagNum++;
  if(aacTagNum == 20)
  {
    //调用直播状态接口,上传状态录制中
    if(save_httpflag)
    {
       recive_httpflag = 1;
       int m_ret = UpdataRecordflag(save_http,recive_httpflag);
    } 
    aacTagNum = 0;
  }
	return 0;
}


//264数据写入.h264文件
int RecordSaveRunnable::Write264data(char *timebuff, char *packetBody, int datasize)
{      

     char flag[] = {0x00,0x00,0x00,0x01};
     char *p = packetBody;
    
     int a = 0;

     if(((packetBody[0] & 0x0f) == 7)&& ((packetBody[1] & 0x0f) == 0)) 
     {  
   	     p = p + 11;
	     char sps[2]= {};
	     strncpy(sps,p,2);
	     //printf("获取sps长度 ：%x %x\n" ,sps[0],  sps[1]);
	     char *s = (char*)&a;
	     *(s+1) = *(p);
	     *(s) = *(p+1);
        //printf("获取sps长度 ：%x %x\n" ,*(p), *(p+1));
	     //printf("获取sps长度 ：%x\n" ,a);
	     //printf("获取sps int长度 ：%d\n" ,a);

      //写入sps数据
  	  p = p + 2;
	    if(4 != fwrite(flag, sizeof(char), 4, vfile))
      {
         return 1;
      }

      //shijiancuo
      // if(4 != fwrite(timebuff, sizeof(char), 4 ,vfile))
      // {
      //    return 2;
      // }

      //printf("获取sps 11 int长度\n");
      if(a != fwrite(p, sizeof(char), a, vfile))
      {
         return 3;
      }
        //printf("获取sps 22 int长度\n" ,a);
	    p = p +  a + 1;
	  
        int b = 0;
	    s = (char*)&a;
	    *(s+1) =*(p);
	    *(s) = *(p+1);
        
        //写入pps数据
	    p = p+2;
	    if(4 != fwrite(flag, sizeof(char), 4, vfile))
        {
         return 4;
        }

        //shijiancuo
        // if(4 != fwrite(timebuff, sizeof(char), 4 ,vfile))
        // {
        //    return 5;
        // }

       if(a != fwrite(p, sizeof(char), a, vfile))
       {
         return 6;
       }
			
     }else
     {
	     p = p + 5;
	 
	     int a = 0;
	     char *s = (char*)&a;
	     *(s) = *(p+3);
	     *(s+1) = *(p+2);
	     *(s+2) = *(p+1);
	     *(s+3) = *(p);
          
       //printf("写入的数据长度: %d %d\n",a, datasize - 9);
       p = p + 4;
    
	   if(4 != fwrite(flag, sizeof(char), 4, vfile))
      {
         return  7;
      }

      //shijiancuo
      // if(4 != fwrite(timebuff, sizeof(char), 4 ,vfile))
      // {
      //    return 8;
      // }

      if((datasize - 9) != fwrite(p, sizeof(char), datasize - 9, vfile))
      {
         return  9;
      }
   }
   return 0;

}


//获取直播流tag数据进行写文件
int RecordSaveRunnable::WriteFile(char *tagbuffer, int bufferSize,bool first)
{

	//printf("shuji11:%s %d\n",tagbuffer ,bufferSize);
	//fwrite(tagbuffer, 1, bufferSize, wfile);

    char buffsize[3]= {};
    int datasize  = 0;
    int tagesize = 0;
   
    int i= 0;
    int m = 0;
    int read = 0;

    int mread = 0;

    char *p = tagbuffer;

    int Tagsize = bufferSize;

    while(mread < Tagsize)
    {  

       //判断是否为第一个tag 
       if(firstflag)
       {
         p = p + 13;  //香港卫视 337  13;
         mread += 13;
         firstflag = false;     
       }
       
       char *buff = (char*)malloc(11*sizeof(char));
       memset(buff , 0 , 11);
       if(NULL == buff)
       {
            LOG(ERROR) << "tag buff 申请内存失败!";
            continue;
       }
       memcpy(buff,p,11);
     
       //printf("获取数据长度 3字节:%x %x %x\n", buff[1],buff[2], buff[3]);
       char *s = (char*)&datasize;
       *(s) = *(buff+3);
       *(s+1) = *(buff+2);
       *(s+2) = *(buff+1);

       //获取 shijiancuo
       char *timebuff = (char*)malloc(4*sizeof(char));
       char *t_buff = buff;
       t_buff = t_buff+4;
       if(NULL == timebuff)
       {
           LOG(ERROR) << "timebuff 申请内存失败!";
           continue;
       }

       memset(timebuff, 0 ,4);
       memcpy(timebuff, t_buff, 4);

       p = p+11;
       mread += 11;     
       char *databuff = (char*)malloc(datasize*sizeof(char));
       memset(databuff , 0 , datasize);
       if(NULL == databuff)
       {
           LOG(ERROR) << "tag databuff 申请内存失败!";
           continue;
       }
       memcpy(databuff,p,datasize);
      
       p = p+ datasize;
       mread += datasize;

       char *tage = (char*)malloc(4*sizeof(char));
       memset(tage , 0 , 4);
       if(NULL == tage)
       {
           LOG(ERROR) << "tag tage 申请内存失败!";
           continue;
       }
       memcpy(tage,p,4);

       p = p+4;
       mread += 4;

       s = (char*)&tagesize; 
       *(s) = *(tage+3);
       *(s+1) = *(tage+2);
       *(s+2) = *(tage+1);
       *(s+3) = *(tage+0);
     
       //printf("获取tag长度 四字节: %x %x %x %x\n",  tage[0],tage[1], tage[2],tage[3]);
       //printf("tag长度:  总长度 : %d %d \n",  tagesize ,Tagsize); 

      //判断是否为一个完整的tag
      int resCode = 0;
      if(tagesize  == datasize + 11)
      {
            if(buff[0] == 0x09) // 视频 
            { 
                //视频首帧的时间戳
                if(m_H264Frist)
                {
                   int i =0;

                   for(i=0;i<4;i++) Aaacfirst_timeStamp|=((unsigned int)timebuff[3-i]&0xFFu)<<(i*8);

                   printf("0x%x,%u\n",Aaacfirst_timeStamp,Aaacfirst_timeStamp);

                   m_H264Frist = false;

                   LOG(INFO)<<"视频首帧时间戳: "<<Aaacfirst_timeStamp<<"  "<< "直播ID: "<<m_parmData->recordSaveID;
                }

                if(!runningp)
                {
                   int i =0;

                   for(i=0;i<4;i++) H264End_timeStamp|=((unsigned int)timebuff[3-i]&0xFFu)<<(i*8);

                   //printf("0x%x,%u\n",H264End_timeStamp,H264End_timeStamp);

                   if(AaacEnd_timeStamp != 0)
                   {
                   	    unsigned int start = UN_ABS(Aaacfirst_timeStamp,H264first_timeStamp);
                        unsigned int end = UN_ABS(AaacEnd_timeStamp, H264End_timeStamp);

                        if(UN_ABS(end,start) > 500)
                        { 
                        	 LOG(ERROR)<<"时间戳已发生异常，开始停止写入 直播ID: "<<m_parmData->recordSaveID;
                             m_writeAaacH264 = false;
                        }
                   }
                }

                if(m_writeAaacH264)
                {
                	if(0 != Write264data(timebuff ,databuff,datasize))
                    {
                       LOG(ERROR) << "视频tag写入失败 直播ID: "<<m_parmData->recordSaveID;
                    }
                }
            }else if(buff[0] == 0x08) //音频
            {
               //音频首帧的时间戳
               if(m_AacFrist)
               {
                   for(i=0;i<4;i++) H264first_timeStamp|=((unsigned int)timebuff[3-i]&0xFFu)<<(i*8);

                   printf("0x%x,%u\n",H264first_timeStamp,H264first_timeStamp);

                   m_AacFrist  = false;

                   LOG(INFO)<<"音频首帧时间戳: "<<H264first_timeStamp<<"  "<< "直播ID: "<<m_parmData->recordSaveID;
               }

               if(!runningp)
               {
                   int i =0;

                   for(i=0;i<4;i++) AaacEnd_timeStamp|=((unsigned int)timebuff[3-i]&0xFFu)<<(i*8);

                   printf("0x%x,%u\n",AaacEnd_timeStamp,AaacEnd_timeStamp);

                   if(H264End_timeStamp != 0)
                   {
                      
                      unsigned int start = UN_ABS(Aaacfirst_timeStamp,H264first_timeStamp);
                      unsigned int end = UN_ABS(AaacEnd_timeStamp,H264End_timeStamp);
                      
                      if(UN_ABS(end,start) > 500)
                      { 
                         LOG(ERROR)<<"时间戳已发生异常，开始停止写入  直播ID: "<<m_parmData->recordSaveID;;
                         m_writeAaacH264 = false;
                      }
                  }
               }

               if(m_writeAaacH264)
               {
               	  if(0 != WriteAac(timebuff ,databuff, datasize))
                  {
                     LOG(ERROR) << "音频tag写入失败 直播ID: "<<m_parmData->recordSaveID;
                  }
               }
             
            }else if(buff[0] == 0x12) //白板
            {
              bool ok = ExtractDefine(timebuff ,databuff, datasize , 0);
              if(!ok)
              {
                  LOG(ERROR) << "白板tag写入失败 直播ID: "<<m_parmData->recordSaveID;
              }
           }
      }

      free(buff);
      buff = NULL;
      free(databuff);
      databuff = NULL;
      free(tage);
      tage = NULL;
      free(timebuff);
      timebuff = NULL;
   
  }   
  return 0;     
}


RecordSaveRunnable::~RecordSaveRunnable()
{  
      if(NULL != m_pRtmp)
      {
	      RTMP_Free(m_pRtmp);
	      m_pRtmp = NULL;
      }

      if (NULL != buf)
      {
	     free(buf);
         buf = NULL;
      }

      if(NULL != rtmpQueue)
      {
          free(rtmpQueue);
          rtmpQueue = NULL;
      }

      if(NULL != m_parmData)
      {
         if(NULL != m_parmData->recordSaveID)
         {
            free(m_parmData->recordSaveID);
         	  m_parmData->recordSaveID = NULL;
         }

         if(NULL != m_parmData->rtmpUrl)
         {
         	  free(m_parmData->rtmpUrl);
         	  m_parmData->rtmpUrl = NULL;
         }

         free(m_parmData);
         m_parmData = NULL;
     }
    
     if(NULL != recive_http)
     {
        delete(recive_http);
        recive_http = NULL;
     }

     if(NULL != save_http)
     {
        delete(save_http);
        save_http = NULL;
     }
    
    
     fclose(afile);
     fclose(vfile);
     fclose(wfile); 

     afile = NULL;
     vfile = NULL;
     wfile = NULL;

}
