#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <iostream>
#include <list>
#include <map>

using namespace std;

std::string  aacStr = ".aac";
std::string  h264Str = ".h264";
std::string  jsonStr = ".json";
std::string  mergeStr = "merge.";

std::list<string> bathList;

std::map<int, string> AacMap;
std::map<int, string> H264Map;
std::map<int, string> JsonMap;

typedef pair<int , string> PAIR;

ostream& operator<<(ostream& out, const PAIR& p) {
  return out << p.first << "\t" << p.second;
}

void mergeFile(char *basePath, std::map<int, string> &H264Map);

int readFolderList(char *basePath)
{
    DIR *dir;
    struct dirent *ptr;
    char base[1000];
 
    if ((dir=opendir(basePath)) == NULL)
    {
        perror("Open dir error...");
        exit(1);
   }

   char *livepos = basePath+2;
   string liveID = livepos;
   int fileNameSize = liveID.size();
   
   while ((ptr=readdir(dir)) != NULL)
   {       
     if(strcmp(ptr->d_name,".")== 0 || strcmp(ptr->d_name,"..")==0)  //current dir OR parrent dir
     {

           continue;
     }else if(ptr->d_type == 8)  //file
     {     
     	   continue;

     }else if(ptr->d_type == 10)//link file
     {

           //printf("d_name10:%s/%s\n",basePath,ptr->d_name);
           continue;
     }
     else if(ptr->d_type == 4) //dir
     {
            memset(base,0,sizeof(base));
            strcpy(base,basePath);
            strcat(base,"/");
            strcat(base,ptr->d_name);
            printf("d_name4:%s/%s   %s\n",basePath,ptr->d_name ,base);

            std::string pathString = base;

            bathList.push_back(pathString);
            //readFileList(base);
      }
    }
    closedir(dir);

    cout<<"count:"<<bathList.size()<<endl;


    return 0;
 }


int readFileList(char *basePath)
{
    DIR *dir;
    struct dirent *ptr;
    char base[1000];
 
    if ((dir=opendir(basePath)) == NULL)
    {
        perror("Open dir error...");
        exit(1);
   }

   char *livepos = basePath+2;
   string liveID = livepos;
   int fileNameSize = liveID.size();
   
   while ((ptr=readdir(dir)) != NULL)
   {       
     if(strcmp(ptr->d_name,".")== 0 || strcmp(ptr->d_name,"..")==0)  //current dir OR parrent dir
     {

           continue;
     }else if(ptr->d_type == 8)  //file
     {
       	   //printf("d_name8:%s/%s\n",basePath,ptr->d_name);
           string fileName = ptr->d_name;

           if(string::npos != fileName.find(mergeStr))
           {
           	   continue;
           }

           if(string::npos != fileName.find(aacStr))
           { 	   
               string format = fileName;
               format = format.erase(0,fileNameSize);   
               if(aacStr == format)
               {
               	   cout<<0<<"  "<<fileName<<endl;
               	   AacMap.insert(std::pair<int,string>(0,fileName));

               }else
               {
                  format = format.erase(0,2);
                  std::size_t pos = format.find(aacStr);
                  int size = aacStr.size();
                  format = format.erase(pos,size);  

                  format.pop_back();
                  int number = std::atoi(format.c_str() ); 
                  cout<<number<<"  "<<fileName<<endl;
                  AacMap.insert(std::pair<int,string>(number,fileName));

               }

           }else if(string::npos != fileName.find(h264Str))
           {
           	   string format = fileName;
               format = format.erase(0,fileNameSize);   
               if(h264Str == format)
               {
               	   cout<<0<<"  "<<fileName<<endl;
               	   H264Map.insert(std::pair<int,string>(0,fileName));

               }else
               {
                  format = format.erase(0,2);
                  std::size_t pos = format.find(h264Str);
                  int size = h264Str.size();
                  format = format.erase(pos,size);  

                  format.pop_back();          
                  int number = std::atoi(format.c_str() ); 
                  cout<<number<<"  "<<fileName<<endl;
                  H264Map.insert(std::pair<int,string>(number,fileName));
               }
               

           }else if(string::npos != fileName.find(jsonStr))
           {
           	   string format = fileName;
               format = format.erase(0,fileNameSize);   
               if(jsonStr == format)
               {
               	   cout<<0<<"  "<<fileName<<endl;
               	   JsonMap.insert(std::pair<int,string>(0,fileName));

               }else
               {
                  format = format.erase(0,2);
                  std::size_t pos = format.find(jsonStr);
                  int size = jsonStr.size();
                  format = format.erase(pos,size);  

                  format.pop_back();   
                  int number = std::atoi(format.c_str() ); 
                  cout<<number<<"  "<<fileName<<endl;
                  JsonMap.insert(std::pair<int,string>(number,fileName));

               }
               
           }        
     }else if(ptr->d_type == 10)//link file
     {

           printf("d_name10:%s/%s\n",basePath,ptr->d_name);
     }
     else if(ptr->d_type == 4) //dir
     {
     	    continue;
            // memset(base,'\0',sizeof(base));
            // strcpy(base,basePath);
            // strcat(base,"/");
            // strcat(base,ptr->d_name);
            // printf("d_name4:%s/%s\n",basePath,ptr->d_name);
            // readFileList(base);
      }
    }

    // map<int, string>::iterator it = AacMap.begin() ;
    // for(;it!= AacMap.end();it++)
    // {
    //  cout<< it->first <<"+++++++" <<it->second<<endl;
    // }
   
    // it = H264Map.begin() ;
    // for(;it!= H264Map.end();it++)
    // {
    //  cout<< it->first <<"+++++++" <<it->second<<endl;
    // }
    // it = JsonMap.begin() ;
    // for(;it!= JsonMap.end();it++)
    // {
    //  cout<< it->first <<"+++++++" <<it->second<<endl;
    // }

    mergeFile(basePath, AacMap);
    mergeFile(basePath, H264Map);
    mergeFile(basePath, JsonMap);

    closedir(dir);
    return 1;
 }


 void mergeFile(char *basePath,std::map<int, string> &H264Map)
 {

 	  string basePathStr = basePath;
  
    std::map<int ,string>::iterator it;
    FILE  *pFTmp = NULL;
    FILE  *pFSrc = NULL;

    char  szContentBuf[256]  = {0};

    it = H264Map.find(0);
    std::string aacfile = "";
    if(it != H264Map.end())
    {
       aacfile = it->second;

    }else
    {
       aacfile = basePath;
       aacfile = aacfile.erase(0,2);
       aacfile.append(aacStr);     
    }
    cout<<"file:"<<aacfile<<endl;
 
    std::string aacFilename =basePathStr + "/"+ mergeStr + aacfile;
    cout<<"Filename:"<<aacFilename<<endl;
    if(NULL == (pFTmp = fopen(aacFilename.c_str(), "wt")))
    {
        printf("MergeFile:open %s failed!\n", aacFilename.c_str());
        return ;
    }
    it = H264Map.begin();
    for(;it!= H264Map.end();it++)
    {

        cout<< it->first<<"  "<<"wenjianmingzi:" <<it->second<<endl;
        
        string srcFileStr = basePathStr + "/"+ (it->second);
        const char *srcFile = srcFileStr.c_str();

        int c = 0;  

        if (NULL == (pFSrc = fopen(srcFile, "r")))
        {
            printf("MergeFile:open %s failed!\n", srcFile);
            break;
        }

        while((c = getc(pFSrc)) != EOF)  
        {  
                putc(c,pFTmp);  
        } 
    
    }
    fclose(pFTmp);
    pFTmp = NULL;
    H264Map.clear();

 }
    

int main(void)
{
    DIR *dir;
    char basePath[1000] = {0};

    //get the current absoulte path
    // memset(basePath,'\0',sizeof(basePath));
    // getcwd(basePath, 999);
    // printf("the current dir is : %s\n",basePath);

   
   // memset(basePath,'\0',sizeof(basePath));
   // strcpy(basePath,"./log");

   readFileList("./s");

   readFolderList("./s");
   return 0;
}