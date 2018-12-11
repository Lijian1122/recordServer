/*****************************************************
版权所有:北京三海教育科技有限公司
作者：lijian
版本：V0.0.1
时间：2018-09-17
功能：利用rtmpdump庫生成RecordSave.so,接收rtmp传输來的数据保存成.h264文件和.acc文件
******************************************************/

#ifndef RECORDSAVERUNNABLE_H
#define RECORDSAVERUNNABLE_H


#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <getopt.h>
#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sched.h>
#include <sys/time.h>
#include <malloc.h>
#include <time.h>
#include <iostream>
#include <errno.h>
#include <sys/stat.h> 

#include "../Httpclient/LibcurClient.h"
#include "../json.hpp"

using json = nlohmann::json;

extern "C"
{
  #include "librtmp/rtmp_sys.h"
  #include "librtmp/log.h"
  #include "Queue.h"
}

#define HTON16(x)  ((x>>8&0xff)|(x<<8&0xff00))
#define HTON24(x)  ((x>>16&0xff)|(x<<16&0xff0000)|x&0xff00)
#define HTON32(x)  ((x>>24&0xff)|(x>>8&0xff00)| (x << 8 & 0xff0000) | (x << 24 & 0xff000000))

#define UN_ABS(a,b) (a>=b?a-b:b-a)

#define IPPORT "http://192.168.1.205:8080/live/"  //"/home/record/recordFile/"
#define FILEFOLDER "./recordFile/"
//#define LOGFOLDER "./record_log/"

#define TESTURL "rtmp://www.bj-mobiletv.com:8000/live/FreeManCamera2018003"

typedef struct AdtsHeader
{
	unsigned char SamplIndex1: 3;
	unsigned char OBjecttype: 5;//2
	unsigned char other: 3;//000
	unsigned char channel: 4;
	unsigned char SamplIndex2: 1;
}AdtsHeader;

typedef struct AdtsData
{
	unsigned char check1;
	unsigned char protection : 1;//误码校验1
	unsigned char layer : 2;//哪个播放器被使用0x00
	unsigned char ver : 1;//版本 0 for MPEG-4, 1 for MPEG-2
	unsigned char check2 : 4;
	unsigned char channel1 : 1;
	unsigned char privatestream : 1;//0
	unsigned char SamplingIndex : 4;//采样率
	unsigned char ObjectType : 2;
	unsigned char length1 : 2;
	unsigned char copyrightstart : 1;//0
	unsigned char copyrightstream : 1;//0
	unsigned char home : 1;//0
	unsigned char originality : 1;//0
	unsigned char channel2 : 2;
	unsigned char length2;
	unsigned char check3 : 5;
	unsigned char length3 : 3;
	unsigned char frames : 2;//超过一块写
	unsigned char check4 : 6;
}AdtsData;

typedef struct ParmData
{
     char *recordSaveID; 
     char *rtmpUrl;     
}ParmData;


class RecordSaveRunnable
{

public:
    RecordSaveRunnable(ParmData *pdata);
    ~RecordSaveRunnable();

    //启动录制线程初始化
    int CmdRecord(); 

    int StopRecord();

protected:

    void *rtmpRecive_f();
    void *rtmpSave_f();

    //静态函数调用非静态方法
    static void *Recive_fun(void* arg);
    static void *Save_fun(void *arg);

private:

    //获取直播流tag数据进行写文件
    int WriteFile(char *tagbuffer, int bufferSize,bool first);

    //264数据写入.h264文件
    int Write264data(char *timebuff,char *packetBody, int datasize);

    //aac数据写入aac文件
    int WriteAac(char *timebuff ,char *data, int len);

    //bool WriteWhiteData(char *data, int len);

    //自定义白板数据写入json文件
    bool ExtractDefine(char *timebuff ,char *data,  int len,  int timestamp);

    //新建文件
    int CreateFileDir(const char *sPathName);
    int CreateFile();

    //解析http返回json
    int ParseJsonInfo(std::string &jsonStr ,std::string &resCodeInfo,std::string &liveinfo);

    //更新录制状态
    int UpdataRecordflag(LibcurClient *recive_http ,int flag);

public:
   //获取参数结构体
   ParmData *m_parmData;

private:

   /*音频解析所需要数据结构*/
   int iHasAudioSpecificConfig;
   AdtsHeader ah;
   AdtsData ad; 

   /*信号量*/
   sem_t bin_sem;
   sem_t bin_blank;

   pthread_t producter_t;  /*生产者线程*/
   pthread_t consumer_t;  /*消费者线程*/

   Queue *rtmpQueue; /*缓冲区队列*/
  
   int runningp; //生产者线程运行标志
   int runningc; //消费者线程运行标志  

   bool firstflag;  //首次解析标志位

   int lRecvBytes;  //总共接收道的数据

   int Iread;   //消费者读Tag次数
   int Iwrite;  //生产者写入Tag次数
   int tagI;    //总共接收到的Tag

   RTMP *m_pRtmp;  //rtmp对象实例化

   char *buf;

   int m_ret;

   LibcurClient *recive_http;
   LibcurClient *save_http;

   bool save_httpflag;  
   
   int  recive_httpflag;  //录制状态

   int  aacTagNum;

   FILE *afile;
   FILE *vfile;
   FILE *wfile;  //baiban

   //音频 视频时间戳
   unsigned int Aaacfirst_timeStamp;
   unsigned int H264first_timeStamp;

   unsigned int AaacEnd_timeStamp;
   unsigned int H264End_timeStamp;

   bool m_AacFrist;
   bool m_H264Frist;

   bool m_writeAaacH264;
};

#endif // RECORDSAVERUNNABLE_H
