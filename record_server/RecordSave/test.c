#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned int i,v;

#define UN_ABS(a,b) (a>=b ?a-b:b-a)

int main() 
{

	char sDec[4] = {0x00,0x00,0x07,0x0d};

    v=0;

    for (i=0;i<4;i++) v|=((unsigned int)sDec[3-i]&0xFFu)<<(i*8);

    printf("0x%x,%d\n",v,v);

    unsigned int a = 20;
    unsigned int b = 40;

    unsigned int c  = 0;
    // if(a > b)
    // {
    //    c =a -b;
    // }else
    // {
    //    c= b -a ;
    // }

    c = UN_ABS(a,b);

    printf("jueduizhi: %u\n", c);

    if(c > 10)
    {
    	printf("2222\n");
    }else
    {
    	printf("1111\n");
    }
    
    return 0;

}
