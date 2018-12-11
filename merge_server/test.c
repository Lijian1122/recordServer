 string format = fileName;
               format = format.erase(0,fileNameSize);   
               if(AACSTR == format)
               {
               	   cout<<0<<"  "<<fileName<<endl;
               	   m_AacMap.insert(std::pair<int,string>(0,fileName));

               }else
               {
                  int found = fileName.find_first_of("(");
                  string foundstr = fileName.substr(0,found);
                  if(foundstr != liveIDStr)
                  {
                      printf("filename is not matching folder!\n");
                      LOG(ERROR)<<"执行任务 liveID:"<<basePath<<"  filename is not matching folder";
                      continue;
                  }
                  format = format.erase(0,2);
                  std::size_t pos = format.find(AACSTR);
                  int size = AACSTR.size();
                  format = format.erase(pos,size);  

                  format.pop_back();
                  int number = atoi(format.c_str() ); 
                  cout<<number<<"  "<<fileName<<endl;
                  m_AacMap.insert(std::pair<int,string>(number,fileName));

               }

                string format = fileName;
                format = format.erase(0,fileNameSize);   
                if(H264STR == format)
                {
                   m_H264Map.insert(std::pair<int,string>(0,fileName));

                }else
                {

                   int found = fileName.find_first_of("(");
                   string foundstr = fileName.substr(0,found);
                   if(foundstr != liveIDStr)
                   {
                       printf("filename is not matching folder!\n");
                       LOG(ERROR)<<"执行任务 liveID:"<<basePath<<"  filename is not matching folder";
                       continue;
                   }
                  format = format.erase(0,2);
                  std::size_t pos = format.find(H264STR);
                  int size = H264STR.size();
                  format = format.erase(pos,size);  

                  format.pop_back();          
                  int number = atoi(format.c_str() ); 
                  cout<<number<<"  "<<fileName<<endl;
                  m_H264Map.insert(std::pair<int,string>(number,fileName));
               }
               string format = fileName;
               format = format.erase(0,fileNameSize);   
               if(JSONSTR == format)
               {
                   cout<<0<<"  "<<fileName<<endl;
                   m_JsonMap.insert(std::pair<int,string>(0,fileName));

               }else
               {
                   int found = fileName.find_first_of("(");
                   string foundstr = fileName.substr(0,found);
                   if(foundstr != liveIDStr)
                   {
                     printf("filename is not matching folder!\n");
                     LOG(ERROR)<<"执行任务 liveID:"<<basePath<<"  filename is not matching folder";
                     continue;
                   }
                  format = format.erase(0,2);
                  std::size_t pos = format.find(JSONSTR);
                  int size = JSONSTR.size();
                  format = format.erase(pos,size);  

                  format.pop_back(); 
                  int number = atoi(format.c_str() ); 
                  cout<<number<<"  "<<fileName<<endl;
                  m_JsonMap.insert(std::pair<int,string>(number,fileName));
               }