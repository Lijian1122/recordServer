#include <iconv.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Httpclient/LibcurClient.h"

#define OUTLEN 1024

using namespace std;

// 代码转换操作类
// class CodeConverter {
// private:
// iconv_t cd;
// public:
// // 构造
// CodeConverter(const char *from_charset,const char *to_charset) {
// cd = iconv_open(to_charset,from_charset);
// }

// // 析构
// ~CodeConverter() {
// iconv_close(cd);
// }

// // 转换输出
// int convert(char *inbuf,int inlen,char *outbuf,int outlen) {
// char **pin = &inbuf;
// char **pout = &outbuf;

// memset(outbuf,0,outlen);
// return iconv(cd,pin,(size_t *)&inlen,pout,(size_t *)&outlen);
// }
// };


unsigned char * urlencode(char const *s, int len, int *new_length)
{

    unsigned char const *from, *end;
    unsigned char *start ,*to;
    from = (unsigned char*)s;
    end = (unsigned char*)s + len;
    start = to = (unsigned char *) malloc(3 * len + 1);

    char hexchars[] = "0123456789ABCDEF";

    while (from < end) {
       char c = *from++;

        if (c == ' ') {
            *to++ = '+';
        } else if ((c < '0' && c != '-' && c != '.')
                   ||(c < 'A' && c > '9')
                   ||(c > 'Z' && c < 'a' && c != '_')
                   ||(c > 'z')) {
            to[0] = '%';
            to[1] = hexchars[c >> 4];
            to[2] = hexchars[c & 15];
            to += 3;
        } else {
            *to++ = c;
        }
    }
    *to = 0;
    if (new_length) {
        *new_length = to - start;
    }
    return (unsigned char *) start;

}


int main(int argc, char **argv)
{
   char *in_utf8 = "hhhtttttt姝ｅ";
   char *in_gb2312 = "录制服务05";
   //char *in_gb2312   =  "http://192.168.1.205:8080/live/server_create?serverType=4&serverName=日志输出01&netFlag=1&serverIp=192.168.1.206:8000";
   char out[OUTLEN];

   // utf-8-->gb2312
   // CodeConverter cc = CodeConverter("utf-8","gb2312");
   // cc.convert(in_utf8,strlen(in_utf8),out,OUTLEN);
   // cout << "utf-8-->gb2312 in=" << in_utf8 << ",out=" << out << endl;

   // gb2312-->utf-8
  // CodeConverter cc2 = CodeConverter("gb2312","utf-8");
  // cc2.convert(in_gb2312,strlen(in_gb2312),out,OUTLEN);
  // cout << "gb2312-->utf-8 in=" << in_gb2312 << ",out=" << out << endl;


  //  //注册录制服务接口  

  // LibcurClient *m_httpclient = new LibcurClient;
  // int main_ret = m_httpclient->HttpGetData(out);
  // if(main_ret != 0)
  // {

  //      cout<<main_ret<<endl;
  //      return main_ret;

  // }else
  // {
  //         std::string res = m_httpclient->GetResdata();
  //         printf("777:%s\n", res.c_str());
 
  // }

   int *new_length = 0;
   unsigned char * dest =  urlencode(in_gb2312,strlen(in_gb2312), new_length);

   printf("%s\n", dest);

   printf("%d\n", *new_length);

}