#ifndef QUEUE_H_LY
#define QUEUE_H_LY


#ifdef __cplusplus
extern "C" {
#endif

#define TRUE 1
#define FALSE 0
#define MaxSize 801

//typedef int Elemtype;
typedef struct RtmpRStruct
{
       char *prtmpdata; //
       int mread;

}RtmpRStruct;

/*
本页面主要是对在程序中使用到的类型做定义
如结点类型,数据类型
*/
typedef struct node
{
	//结点的数据域
	RtmpRStruct data;
	
	//结点的指针域,存放下一个结点的地址
	struct node *next;
}SNode;


/*队列的基本结构*/
typedef struct 
{
	//存放队列元素
	RtmpRStruct q[MaxSize];
	
	//头指针
	int front;

	//尾指针(尾元素的下一个位置)
	int rear;
}Queue;



Queue * InitQueue();  //队列初始化

int EnQueue(Queue *Q,RtmpRStruct x); //进队列

int DeQueue(Queue *Q,RtmpRStruct *x); //出队列

int QueueEmpty(Queue Q);  //判断队列是否为空

int QueueCount(Queue *HQ);  //统计队列元素个数

int QueueFull(Queue Q);   //判断是否队列满了

#ifdef __cplusplus
}
#endif

#endif   
