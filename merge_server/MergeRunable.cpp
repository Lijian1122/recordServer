#include "MergeRunable.h"

MergeRunable::MergeRunable(std::string path)
{
	 m_basePath =  path;

   m_fileNameSize = 0;
}

int MergeRunable::run()
{
  
  int rescode = readFileList();

  if(0 != rescode)
  {
      LOG(ERROR)<<"执行任务 liveID:"<<m_basePath<<" 失败"<< "  rescode:"<<rescode;

      return rescode;
  }

  LOG(INFO)<<"执行任务 liveID:"<<m_basePath<<"  合成成功"<< "  rescode:"<<rescode;

  rescode = UpdataRecordflag(rescode);
  return rescode;
}

 //更新录制状态
int MergeRunable::UpdataRecordflag(int flag)
{

    LibcurClient  m_httpclient;

    std::string resCodeInfo;
    std::string urlparm = "live_update?liveId=";
   
    urlparm.append(m_liveIDStr);
    urlparm.append("&mixFlag=");
          
    char flagStr[10] ={};
    snprintf(flagStr, sizeof(flagStr), "%d",flag);
    urlparm.append(flagStr);
    urlparm.append("&operateId=8888");
  
    std::string updataUrl = IPPORT + urlparm;
    LOG(INFO)<<"UpdataMergeUrl:"<<updataUrl;

    int m_ret = m_httpclient.HttpGetData(updataUrl.c_str());
   
    std::string resData = m_httpclient.GetResdata();

    m_ret = ParseJsonInfo(resData, resCodeInfo);

    return m_ret;
}

//解析http返回json
int MergeRunable::ParseJsonInfo(std::string &jsonStr ,std::string &resCodeInfo)
{
    int main_ret = 0;

    std::cout<<"parse json:"<<jsonStr<<endl;
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
          
            LOG(INFO)<<"执行任务 liveID:"<<m_basePath<<"  返回http接口信息  msg:"<<resCodeInfo;
            return main_ret;
         }
    }else
    {
        LOG(ERROR)<<"执行任务 liveID:"<<m_basePath<<" 返回http 接口数据不全!";
        main_ret = 1;
        return main_ret;
    }
}


int MergeRunable::readFileList()
{
	  DIR *dir;
    struct dirent *ptr;
    int rescode = 0;

    if((dir=opendir(m_basePath.c_str())) == NULL)
    {
        perror("Open dir error...");
        LOG(ERROR)<<"执行任务 liveID:"<<m_basePath<<"  Open dir error...";
        return -1;
    }

    string liveID = m_basePath.c_str();
    
    int found = liveID.find_last_of("/");
    string ss = liveID.substr(0,found);
    m_liveIDStr = liveID.substr(found+1);
    m_fileNameSize = m_liveIDStr.size();
   
    while((ptr=readdir(dir)) != NULL)
    {
       usleep(3);  

       if(strcmp(ptr->d_name,".")== 0 || strcmp(ptr->d_name,"..")==0)  //current dir OR parrent dir
       {

            continue;
       }else if(ptr->d_type == 8)  //file
       {
           string fileName = ptr->d_name;

           if(string::npos != fileName.find(MERGESTR))
           {
           	   continue;
           }

           if(string::npos != fileName.find(AACSTR))
           { 	   
              setFileNameToMap(m_AacMap,fileName,AACSTR);

           }else if(string::npos != fileName.find(H264STR))
           {
              setFileNameToMap(m_H264Map , fileName , H264STR);

           }else if(string::npos != fileName.find(JSONSTR))
           {
           	  setFileNameToMap(m_JsonMap , fileName , JSONSTR);
           }
       }else if(ptr->d_type == 10)//link file
       {
          continue;
       }
       else if(ptr->d_type == 4) //dir
       {
     	   continue;
         
       }
    }

    closedir(dir);

    if(!m_AacMap.empty())
    {
       rescode = mergeFile(m_AacMap,AACSTR);
    }
    if(!m_H264Map.empty())
    {
       rescode = mergeFile(m_H264Map,H264STR);
    }

    if(!m_JsonMap.empty())
    {
       rescode = mergeFile(m_JsonMap,JSONSTR);
    }
  
    return rescode;

}


int MergeRunable::setFileNameToMap(std::map<int, string> &Map , std::string &fileName , std::string &fileType)
{ 
        
  string format = fileName;
  format = format.erase(0,m_fileNameSize);   
  if(fileType == format)
  {
    cout<<0<<"  "<<fileName<<endl;
    Map.insert(std::pair<int,string>(0,fileName));
    return 0;

  }else
  {

    int found = fileName.find_first_of("(");
    string foundstr = fileName.substr(0,found);
    if(foundstr != m_liveIDStr)
    {
         printf("filename is not matching folder!\n");
         LOG(ERROR)<<"执行任务 liveID:"<<m_basePath<<"  filename is not matching folder";
         return 1;
    }

    format = format.erase(0,2);
    std::size_t pos = format.find(fileType);
    int size = fileType.size();
    format = format.erase(pos,size);  

    format.pop_back();
    int number = atoi(format.c_str() ); 
    cout<<number<<"  "<<fileName<<endl;
    Map.insert(std::pair<int,string>(number,fileName));

    return 0;

  }
}

int MergeRunable::mergeFile(std::map<int, string> &Map, std::string filetype)
{

    pthread_t tid = pthread_self();
    printf("[tid: %lu]\trun \n: ", tid);
 	  string basePathStr = m_basePath.c_str();
  
    std::map<int ,string>::iterator it;
    FILE  *pFTmp = NULL;
    FILE  *pFSrc = NULL;

    //获取路径中的liveID
    // string path = basePath.c_str();
    // int found = path.find_last_of("/");
    // string liveIDStr = path.substr(found+1);
    
    std::string mergeFilename =basePathStr + "/"+ MERGESTR + m_liveIDStr + filetype;
    cout<<"merge Filename:"<<mergeFilename<<endl;
    LOG(INFO)<<"执行任务 liveID:"<<m_basePath<<"   mergeFilename:"<<mergeFilename;
    if(NULL == (pFTmp = fopen(mergeFilename.c_str(), "ab+")))
    {
        printf("MergeFile:open %s failed!\n", mergeFilename.c_str());
        LOG(ERROR)<<"执行任务 liveID:"<<m_basePath<<"  "<<mergeFilename<<"open failed ";
        return -1;
    }
    it = Map.begin();
    for(;it!= Map.end();it++)
    {

        cout<< it->first<<"  "<<"file name:" <<it->second<<endl; 
        LOG(INFO)<<"执行任务 liveID:"<<m_basePath<<"   filename:"<<it->second;

        string srcFileStr = basePathStr + "/"+ (it->second);
        const char *srcFile = srcFileStr.c_str();

        int iLen = 0; 

        if (NULL == (pFSrc = fopen(srcFile, "r")))
        {
            printf("MergeFile:open %s failed!\n", srcFile);
            LOG(ERROR)<<"执行任务 liveID:"<<m_basePath<<"  MergeFile open "<<srcFile<<" failed ";
            break;
        }

        fseek(pFSrc,0L,SEEK_END);   /* 定位到文件末尾 */
        iLen=ftell(pFSrc); /*得到文件大小*/

        int time = iLen/1024;
        int end = iLen%1024;
        char buff[1024] = {0};

        fseek(pFSrc,0L,SEEK_SET); /*定位到文件开头*/
        if(time > 0)
        {
          int i = 0;
          for(;i < time;++i)
          {
              //usleep(3);
              fread(buff,1024,1,pFSrc);
              fwrite(buff,1024,1,pFTmp);
              fseek(pFSrc,1024*(i+1),0);  
              memset(buff,0 ,1024);
          }
        }

        fread(buff,end,1,pFSrc);
        fwrite(buff,end,1,pFTmp);
        
        // while((iLen = getc(pFSrc)) != EOF)  
        // {            
        //    putc(iLen,pFTmp);       
        // } 

        fclose(pFSrc);

        //hecheng hou shanchu benwenjian
        //remove(srcFileStr.c_str()); 
    
    }
    fclose(pFTmp);
    pFTmp = NULL;
    Map.clear();

    return 0;
}

MergeRunable::~MergeRunable()
{
	 
}
