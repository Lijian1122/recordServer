#include<iostream>
#include<stdio.h> 
#include<stdio.h>
#include <string.h>
#include<memory>

#include <locale.h>

#include "Httpclient/LibcurClient.h"

#define IPPORT "http://192.168.1.205:8080/live/";

using namespace std;

unsigned char char_to_hex( unsigned char x ) 
{ 
   return (unsigned char)(x > 9 ? x + 55: x + 48); 
} 
int is_alpha_number_char( unsigned char c ) 
{ 
   if( (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ) 
       return 1; 
   return 0; 
} 
//url编码实现 
void urlencode( unsigned char * src, int  src_len, unsigned char * dest, int  dest_len) 
{ 
    unsigned char ch; 
    int  len = 0; 
 
    while (len < (dest_len - 4) && *src) 
    {

       ch = (unsigned char)*src; 
       if (*src == ' ') 
       { 
         *dest++ = '+'; 
       } 
       else if (is_alpha_number_char(ch) || strchr("=!~*'()", ch)) 
       { 
         *dest++ = *src; 
       } 
       else 
       { 
         *dest++ = '%'; 
         *dest++ = char_to_hex( (unsigned char)(ch >> 4) ); 
         *dest++ = char_to_hex( (unsigned char)(ch % 16) ); 
       }  
       ++src; 
       ++len; 
   } 
   *dest = 0; 
   return ; 
} 


// std::wstring s2ws(const std::string& str)
// {
//   if (str.empty()) 
//   {
//     return L"";
//   }
//   unsigned len = str.size() + 1;
//   setlocale(LC_CTYPE, "en_US.UTF-8");
//   std::unique_ptr<wchar_t[]> p(new wchar_t[len]);
//   mbstowcs(p.get(), str.c_str(), len);
//   std::wstring w_str(p.get());
//   return w_str;
// }

int gbk2utf8(char *utfstr,const char *srcstr,int maxutfstrlen)
{
    if(NULL==srcstr)
    {
        printf("bad parameter 1\n");
        return -1;
    }
    //首先先将gbk编码转换为unicode编码
    if(NULL==setlocale(LC_ALL,"zh_cn.gbk"))//设置转换为unicode前的码,当前为gbk编码
    {
        printf("bad parameter 2\n");
        return -1;
    }
    int unicodelen=mbstowcs(NULL,srcstr,0);//计算转换后的长度
    if(unicodelen<=0)
    {
        printf("can not transfer!!!\n");
        return -1;
    }
    wchar_t *unicodestr=(wchar_t *)calloc(sizeof(wchar_t),unicodelen+1);
    mbstowcs(unicodestr,srcstr,strlen(srcstr));//将gbk转换为unicode
    
    //将unicode编码转换为utf8编码
    if(NULL==setlocale(LC_ALL,"zh_cn.utf8"))//设置unicode转换后的码,当前为utf8
    {
        printf("bad parameter\n");
        return -1;
    }
    int utflen=wcstombs(NULL,unicodestr,0);//计算转换后的长度
    if(utflen<=0)
    {
        printf("can not transfer!!!\n");
        return -1;
    }
    else if(utflen>=maxutfstrlen)//判断空间是否足够
    {
        printf("dst str memory not enough\n");
        return -1;
    }
    wcstombs(utfstr,unicodestr,utflen);
    utfstr[utflen]=0;//添加结束符
    free(unicodestr);
    return utflen;
}

int main(int argc, char **argv)
{
   const char *in = "录制服务06";

   char dest[1024] = {0};

   int ret = gbk2utf8(dest,in,1024);

  //  urlencode((unsigned char*)(in), strlen(in), (unsigned char*)dest, 1024); 

   printf("%s\n", dest);

  //  //注册录制服务接口  

  // std::string url = IPPORT;
  // url.append("server_create?serverType=4&serverName=");
  // url.append(dest);
  // url.append("&netFlag=1&serverIp=192.168.1.206:8000");
  // printf("%s\n", url.c_str());


  // LibcurClient *m_httpclient = new LibcurClient;
  // int main_ret = m_httpclient->HttpGetData(url.c_str());
  // if(main_ret != 0)
  // {

  //      cout<<main_ret<<endl;
  //      return main_ret;

  // }else
  // {
  //   std::string res = m_httpclient->GetResdata();
  //   printf("777:%s\n", res.c_str());
  // }

  
   return  0;

}
