
#include <stdio.h>
#include <malloc.h>
#include "Queue.h"

Queue *InitQueue()
{
	Queue * Q = (Queue *)malloc(sizeof(Queue));

	Q->front = Q->rear = 0;
	return Q;
}


int EnQueue(Queue *Q,RtmpRStruct x)
{
    if((Q->rear + 1)%MaxSize==Q->front)
        return FALSE;
    else
    {
        Q->q[Q->rear] = x;
        Q->rear = (Q->rear+1)%MaxSize;
        return TRUE;
    }
}


int DeQueue(Queue *Q,RtmpRStruct *x)
{
    if(Q->rear==Q->front)
        return FALSE;
    else
    {
        *x = Q->q[Q->front];
        Q->front = (Q->front+1)%MaxSize;
        return TRUE;
    }
}


int QueueEmpty(Queue Q)
{
    if(Q.rear==Q.front)
        return TRUE;
    else
        return FALSE;
}


RtmpRStruct Last(Queue *Q)
{
  
    RtmpRStruct *prElem = NULL;
    Queue *prTempQueue;
    
    prTempQueue = InitQueue();
    while(QueueEmpty(*Q)==1)
    {
        DeQueue(Q,prElem);
        EnQueue(prTempQueue,*prElem);
    }
    while(QueueEmpty(*prTempQueue)==1)
    {
        DeQueue(prTempQueue,prElem);
        EnQueue(Q,*prElem);
    }
    return *prElem;
}

int QueueFull(Queue Q)
{
    if(((Q.rear+1)%MaxSize)==Q.front)
        return TRUE;
    else
        return FALSE;
}
