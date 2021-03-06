// # HTTP client example

// To create an HTTP client, follow this pattern:

// 1. Create an outbound connection by calling `mg_connect_http()`
// 2. Create an event handler function that handles `MG_EV_HTTP_REPLY` event

// Here's an example of the simplest HTTP client.
// Error checking is omitted for the sake of clarity:

// ```c
#include "mongoose.h"
#include <string>
#include <stdio.h>
#include <stdlib.h>

using namespace std;

//#define CONTENTTYPE "Content-Type: application/json;charset=utf-8\r\n" 
#define CONTENTTYPE "Content-Type: application/x-www-form-urlencoded\r\n"  

#define POSTFIELDS "recodID=333333&rtmpurl=rtmp://www.bj-mobiletv.com:8000/live/FreeManCamera002&vfile=vhh.h264&afile=ahh.aac"  

//static const char *url = "http://localhost:8000/startRecord";

//static const char *url  =  "http://localhost:8000/live/record?liveId=s&type=0";
string url  =  "http://localhost:8000/live/record?liveId=";
//static const char *url = "http://www.baidu.com";
static int exit_flag = 0;

static void ev_handler(struct mg_connection *c, int ev, void *p) {
  if (ev == MG_EV_HTTP_REPLY) {
    struct http_message *hm = (struct http_message *)p;
    c->flags |= MG_F_CLOSE_IMMEDIATELY;
    fwrite(hm->message.p, 1, (int)hm->message.len, stdout);
    putchar('\n');
    exit_flag = 1;
  } else if (ev == MG_EV_CLOSE) {
    exit_flag = 1;
  };
}

int main(int argv ,char **argc) 
{

   if(argv < 3)
   {
     printf("please input liveID!\n");
     return 0;
   }

   url.append(argc[1]);
   url.append("&type=");
   url.append(argc[2]);

   printf("url :%s\n", url.c_str());

   struct mg_mgr mgr;

   mg_mgr_init(&mgr, NULL);
   //mg_connect_http(&mgr, ev_handler, url, NULL, NULL);

   mg_connect_http(&mgr, ev_handler, url.c_str(), NULL, NULL);

  //mg_connect_http(&mgr, ev_handler, url, CONTENTTYPE, POSTFIELDS);

  while(exit_flag == 0) 
  {
    mg_mgr_poll(&mgr, 1000);
  }
  mg_mgr_free(&mgr);

  return 0;
}


//See full source code at [HTTP client example](https://github.com/cesanta/mongoose/tree/master/examples/http_client).
