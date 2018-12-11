#ifndef __THREAD_POOL_H
#define __THREAD_POOL_H

#include <vector>
#include <string>
#include <pthread.h>
#include <queue>

#include "MergeRunable.h"
#include "glog/logging.h"

using namespace std;


/*线程池管理类*/
class CThreadPool 
{

public:
    CThreadPool(int threadNum);
    ~CThreadPool();
    int AddTask(MergeRunable *m_task);   //把任务添加到任务队列中
    int StopAll();  //使线程池中的所有线程退出
    int getTaskSize();  //获取当前任务队列中的任务数

private:

    //vector<MergeRunable*> m_vecTaskList;    //任务列表
    std::queue<MergeRunable*> m_vecTaskList;    //任务列表

    bool shutdown;   //线程退出标志
    int m_iThreadNum;   //线程池中启动的线程数
    pthread_t *pthread_id;
  
    static pthread_mutex_t m_pthreadMutex;  //线程同步锁
    static pthread_cond_t m_pthreadCond;    //线程同步条件变量
  
protected:

    int Create();   //创建线程池中的线程

    static void *ThreadFunc(void *arg);  //线程回调函数

    void *MergeFileThread();
    
};

#endif