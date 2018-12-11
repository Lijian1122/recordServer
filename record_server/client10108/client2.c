// # HTTP client example

// To create an HTTP client, follow this pattern:

// 1. Create an outbound connection by calling `mg_connect_http()`
// 2. Create an event handler function that handles `MG_EV_HTTP_REPLY` event

// Here's an example of the simplest HTTP client.
// Error checking is omitted for the sake of clarity:

// ```c
#include "mongoose.h"


//#define CONTENTTYPE "Content-Type: application/json;charset=utf-8\r\n" 
#define CONTENTTYPE "Content-Type: application/x-www-form-urlencoded\r\n"  

#define POSTFIELDS "recodID=333333&rtmpurl=rtmp://www.bj-mobiletv.com:8000/live/0&vfile=vhh.h264&afile=ahh.aac"  

//static const char *url = "http://localhost:8000/startRecord";
//static char *url = "http://101.201.142.11:9705/c/235699359980553248/7792399914543424/9b30cb636c074d19846097b917e9cfad/kh_-2uDz4sp49S4RUAPGD_FDw3T4/3687f43ec2ad5c3de103f3e27e50aac7/7792399914543424/7792399914543424/0/courseInfo.do";

//static const char *url  =  "http://localhost:8000/live/record?liveId=FreeManCamera000&type=0";
//static const char *url = "http://www.baidu.com";
static int exit_flag = 0;
static int count = 0;
static void ev_handler(struct mg_connection *c, int ev, void *p) {
  if (ev == MG_EV_HTTP_REPLY) {
    struct http_message *hm = (struct http_message *)p;
    c->flags |= MG_F_CLOSE_IMMEDIATELY;
    fwrite(hm->message.p, 1, (int)hm->message.len, stdout);
    putchar('\n');
    exit_flag = 1;
    // if(count == 9)
    // {
    // 	exit_flag = 1;
    // }
  } else if (ev == MG_EV_CLOSE) {
  	// if(count == 9)
   //  {
   //  	exit_flag = 1;
   //  }
    exit_flag = 1;
  };
}

int main(void) {
  struct mg_mgr mgr;

  mg_mgr_init(&mgr, NULL);
  //mg_connect_http(&mgr, ev_handler, url, NULL, NULL);
  //int i = 0;
  for(count = 0; count < 10; count++)
  {
      char url[1024];

      exit_flag = 0;

      char numStr[10] ={};
      snprintf(numStr, sizeof(numStr), "%d",count);

      char resStr[100] = "FreeManCamera";
      strcat(resStr,numStr);
      sprintf(url,"http://localhost:8000/live/record?liveId=%s&type=0",resStr);

      mg_connect_http(&mgr, ev_handler, url, NULL, NULL);

      if(exit_flag == 0) 
      {
       mg_mgr_poll(&mgr, 1000);
      }
    //mg_mgr_poll(&mgr, 1000);

      //sleep(1);
  }

  //mg_connect_http(&mgr, ev_handler, url, NULL, NULL);
  //mg_connect_http(&mgr, ev_handler, url, CONTENTTYPE, POSTFIELDS);
  //mg_mgr_poll(&mgr, 1000);

  // while (exit_flag == 0) 
  // {
  //   mg_mgr_poll(&mgr, 1000);
  // }

  mg_mgr_free(&mgr);

  return 0;
}


//See full source code at [HTTP client example](https://github.com/cesanta/mongoose/tree/master/examples/http_client).
