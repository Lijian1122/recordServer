#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/vfs.h>

#include <sys/statvfs.h>
#include <sys/vfs.h>
#include <errno.h>

#define DEFAULT_DISK_PATH "/home"

typedef struct statfs DISK,*pDISK;


void checkdisk(char *path)
{
	    struct statfs diskInfo;
		statfs(path, &diskInfo);
		unsigned long long totalBlocks = diskInfo.f_bsize;
		unsigned long long totalSize = totalBlocks * diskInfo.f_blocks;
		size_t mbTotalsize = totalSize>>20;
		unsigned long long freeDisk = diskInfo.f_bfree*totalBlocks;
		size_t mbFreedisk = freeDisk>>20;
		printf ("%s: total=%dMB, free=%dMB\n",path, mbTotalsize, mbFreedisk);
 
		// statfs("/boot", &diskInfo);
		// totalBlocks = diskInfo.f_bsize;
		// totalSize = totalBlocks * diskInfo.f_blocks;
		// mbTotalsize = totalSize>>20;
		// freeDisk = diskInfo.f_bfree*totalBlocks;
		// mbFreedisk = freeDisk>>20;
		// printf ("/boot  total=%dMB, free=%dMB\n", mbTotalsize, mbFreedisk);
 
		// statfs("/dev/shm", &diskInfo);
		// totalBlocks = diskInfo.f_bsize;
		// totalSize = totalBlocks * diskInfo.f_blocks;
		// mbTotalsize = totalSize>>20;
		// freeDisk = diskInfo.f_bfree*totalBlocks;
		// mbFreedisk = freeDisk>>20;
		// printf ("/dev/shm  total=%dMB, free=%dMB\n", mbTotalsize, mbFreedisk);
}


int main()
{
	char buffer[1024] ;
 
    //获取当前的工作目录，注意：长度必须大于工作目录的长度加一
    char *p = getcwd(buffer , 1024);
    char *dir = NULL;
 
    printf("buffer:%s  p:%s size:%d \n" , buffer , p , strlen(buffer));
    
    //获取当前工作目录的名字
    dir = get_current_dir_name();
    printf("dir:%s \n" , dir);
 
    checkdisk(buffer);
   
	return 0;
}

// //获取包含磁盘空间信息的结构体
// //参数二：要获取磁盘信息的位置
// //返回值：成功返回1，失败返回0
// int getDiskInfo(pDISK diskInfo,const char *path)
// {
//     char dpath[100]=DEFAULT_DISK_PATH;//设置默认位置
//     int flag=0;

//     if(NULL!=path)
//     {
//         strcpy(dpath,path);
//     }

//     if(-1==(flag=statfs(dpath,diskInfo)))//获取包含磁盘空间信息的结构体
//     {
//         perror("getDiskInfo statfs fail");
//         return 0;
//     }

//     return 1;
// }

// //计算磁盘总空间，非超级用户可用空间，磁盘所有剩余空间，计算结果以字符串的形式存储到三个字符串里面，单位为MB
// int calDiskInfo(char *diskTotal,char *diskAvail,char *diskFree,pDISK diskInfo)
// {
//     unsigned long long total=0,avail=0,free=0,blockSize=0;
//     int flag=0;

//     if(!diskTotal&&diskAvail&&diskFree&&diskInfo)
//     {
//         printf("\ncalDiskInfo param null!\n");
//         return 0;
//     }
//     blockSize=diskInfo->f_bsize;//每块包含字节大小
//     total=diskInfo->f_blocks*blockSize;//磁盘总空间
//     avail=diskInfo->f_bavail*blockSize;//非超级用户可用空间
//     free=diskInfo->f_bfree*blockSize;//磁盘所有剩余空间

//     //字符串转换
//     flag=sprintf(diskTotal,"%llu",total>>20);
//     flag=sprintf(diskAvail,"%llu",avail>>20);
//     flag=sprintf(diskFree,"%llu",free>>20);

//     if(-1==flag)
//     {
//         return 0;
//     }
//     return 1;

// }


// int main()
// {
//     DISK diskInfo;
//     char str1[30],str2[30],str3[30];

//     memset(&diskInfo,0,sizeof(DISK));

//     getDiskInfo(&diskInfo,DEFAULT_DISK_PATH);//获取磁盘信息结构体

//     calDiskInfo(str1,str2,str3,&diskInfo);//计算磁盘信息结构体

//     printf("\ntotal:%s avail:%s free%s\n",str1,str2,str3);
//     printf("Hello world!\n");
//     return 0;
// }
