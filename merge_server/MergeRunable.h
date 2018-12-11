#ifndef MERGERUNABLE_H
#define MERGERUNABLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <iostream>
#include <list>
#include <map>

#include "glog/logging.h"
#include "json.hpp"
#include "LibcurClient.h"

using json = nlohmann::json;

using namespace std;

extern string AACSTR ,H264STR ,JSONSTR ,MERGESTR ,RELATIVEPATH ,IPPORT;

class MergeRunable
{
public:
    MergeRunable(std::string path);
    ~MergeRunable();

    int run();

    std::string getLiveID(){return m_basePath;}

private:

   int readFileList();


   int mergeFile(std::map<int, string> &Map ,std::string filetype);

   //找到文件名插入map
   int setFileNameToMap(std::map<int, string> &Map , std::string &fileName , std::string &fileType);

   //更新合成状态
   int UpdataRecordflag(int flag);

   //解析http返回json
   int ParseJsonInfo(std::string &jsonStr ,std::string &resCodeInfo);

private:

   std::map<int, string> m_AacMap;
   std::map<int, string> m_H264Map;
   std::map<int, string> m_JsonMap;
	
   std::string m_basePath;
   
   std::string m_liveIDStr;

   int m_fileNameSize;
};

#endif // MERGERUNABLE_H