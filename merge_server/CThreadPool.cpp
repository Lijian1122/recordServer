#include "CThreadPool.h"
#include <cstdio>


//静态成员初始化
pthread_mutex_t CThreadPool::m_pthreadMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t CThreadPool::m_pthreadCond = PTHREAD_COND_INITIALIZER;

//线程管理类构造函数
CThreadPool::CThreadPool(int threadNum) 
{
    m_iThreadNum = threadNum;
    shutdown = false;
    Create();
}

void *CThreadPool::MergeFileThread()
{
    pthread_t tid = pthread_self();
    while(1)
    {
        pthread_mutex_lock(&m_pthreadMutex);
        //如果队列为空，等待新任务进入任务队列
        while(m_vecTaskList.empty() && !shutdown)
            pthread_cond_wait(&m_pthreadCond, &m_pthreadMutex);
        
        //关闭线程
        if(shutdown)
        {
            pthread_mutex_unlock(&m_pthreadMutex);
            printf("[tid: %lu]\texit\n", pthread_self());
            LOG(INFO)<<"线程池 线程 "<<tid<<" 即将退出";
            pthread_exit(NULL);
        }

        printf("[tid: %lu]\trun: ", tid);

        MergeRunable *task = move(m_vecTaskList.front());
        m_vecTaskList.pop();

        // vector<MergeRunable*>::iterator iter = m_vecTaskList.begin();
        // //取出一个任务并处理之
        // MergeRunable* task = *iter;
        // if(iter != m_vecTaskList.end())
        // {
        //     task = *iter;
        //     m_vecTaskList.erase(iter);
        // }

        pthread_mutex_unlock(&m_pthreadMutex);
        
        LOG(INFO)<<" 开始执行任务 liveID:"<<task->getLiveID();
        
        int rescode = task->run();    //执行任务

        if(0 != rescode)
        {
              LOG(ERROR)<<" 执行任务 liveID:"<<task->getLiveID() <<"失败"<< "  rescode:"<<rescode;
        }
        printf("[tid: %lu]\tidle %d\n", tid , rescode);
        LOG(INFO)<<" 执行任务 liveID:"<<task->getLiveID() <<"  完成";

        if(NULL != task)
        {
           delete task;
           task = NULL;
        }      
    } 
    return (void*)0;
}
//线程回调函数
void *CThreadPool::ThreadFunc(void* arg) 
{
    return static_cast<CThreadPool*>(arg)->MergeFileThread();
}

//往任务队列里添加任务并发出线程同步信号
int CThreadPool::AddTask(MergeRunable *task) 
{ 
    pthread_mutex_lock(&m_pthreadMutex);   
    //m_vecTaskList.push_back(task);  
    m_vecTaskList.emplace(task);

    pthread_mutex_unlock(&m_pthreadMutex);  
    pthread_cond_signal(&m_pthreadCond);    

    return 0;
}

//创建线程
int CThreadPool::Create() 
{ 

    printf("I will create %d threads.\n", m_iThreadNum);
    pthread_id = new pthread_t[m_iThreadNum];
    for(int i = 0; i < m_iThreadNum; i++)
    {
        if(0 != (pthread_create(&pthread_id[i], NULL, ThreadFunc, (void *)this)))
        {
             LOG(ERROR)<<"线程池启动第 "<<m_iThreadNum<<" 个线程失败";
        }
    }

    LOG(INFO)<<"线程池已启动 "<<m_iThreadNum<<" 个线程";
        
    return 0;
}

//停止所有线程
int CThreadPool::StopAll()
{    
    //避免重复调用
    if(shutdown)
        return -1;
    printf("Now I will end all threads!\n\n");

    LOG(INFO)<<"线程池开始停止所有线程...";
    
    //唤醒所有等待进程，线程池也要销毁了
    shutdown = true;
    pthread_cond_broadcast(&m_pthreadCond);
    
    //清楚僵尸
    for(int i = 0; i < m_iThreadNum; i++)
    {
       if(0 != (pthread_join(pthread_id[i], NULL)))
       {
           LOG(ERROR)<<"线程池停止第 "<<i<<" 个线程失败";
       }
    }

    LOG(INFO)<<"线程池已停止 "<<m_iThreadNum<<" 个线程";
    
    if(NULL != pthread_id)
    {
       delete[] pthread_id;
       pthread_id = NULL;
    }

 
    //销毁互斥量和条件变量
    pthread_mutex_destroy(&m_pthreadMutex);
    pthread_cond_destroy(&m_pthreadCond);
    
    return 0;
}

//获取当前队列中的任务数
int CThreadPool::getTaskSize() 
{    
    int size= 0;
    pthread_mutex_lock(&m_pthreadMutex);
    size = m_vecTaskList.size();
    pthread_mutex_unlock(&m_pthreadMutex);
    return size;
}

CThreadPool::~CThreadPool()
{

}