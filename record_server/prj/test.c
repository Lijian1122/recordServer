#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
 
#include <stdlib.h>  
#include <unistd.h>

int CreateFileDir(const char *sPathName)  
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
                   printf("bucunzai %s\n",DirName);
                  if(mkdir(DirName, 0755)==-1)  
                  {   
                      printf("mkdir   error\n");   
                      return -1;   
                  }  
                  printf("chuangjianchenggong\n");
              }  
              DirName[i] = '/';  

          }  
      }  

      printf("chuangjian %s\n",DirName);
      return 0;  
} 

int main(){
    CreateFileDir("./lijian/");

    return 0;
}