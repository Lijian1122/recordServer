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

extern "C"
{
  #include "librtmp/rtmp_sys.h"
  #include "librtmp/log.h"
  #include "Queue.h"
  #include <sys/time.h>
  #include <time.h>
}

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
     int client_sock;
     char *rtmpUrl;
     char *vFileStr;
     char *aFileStr;
     int resCode;   //fanhuizhi
}ParmData;

class RecordSaveRunnable
{
public:
    RecordSaveRunnable(ParmData *pdata);
    ~RecordSaveRunnable();

    //启动录制线程初始化
    int CmdRecord(); 

protected:

    void *rtmpRecive_f(int r_ret);
    void *rtmpSave_f();

    void sigIntHandler(int sig);

    //静态函数调用非静态方法
    static void *Recive_fun(void* arg);
    static void *Save_fun(void *arg);


private:

    //获取直播流tag数据进行写文件
    int writeFile(char *tagbuffer, int bufferSize,bool first);

    //264数据写入.h264文件
    void write264data(char *packetBody, int datasize);

    //aac数据写入aac文件
    int WriteAac(char *data, int len);

private:

   /*音频解析所需要数据结构*/
   int iHasAudioSpecificConfig;
   AdtsHeader ah;
   AdtsData ad; 

   ParmData *m_parmData;   //获取参数结构体

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

   static int m_ret;

};

#endif // RECORDSAVERUNNABLE_H
