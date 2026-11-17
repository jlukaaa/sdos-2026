#include <stdio.h>
#include <math.h>
#include "../src/eq.h"
#include "../src/eq_rt.h"
int main(void){
  int v,i; float maxo=0;
  eq_rt_init(400e6f, 48000.0f);
  for (v=0; v<5; v++){
    g_req_version=v; g_req_K=(v==0)?3:200; eq_rt_poll();
    for(i=0;i<48000;i++){ float l=0.9f*sinf(2*3.14159f*1000*i/48000.f), r=0.9f*sinf(2*3.14159f*60*i/48000.f);
      eq_rt_isr_begin(); eq_rt_process(&l,&r); eq_rt_isr_end();
      if (fabsf(l)>maxo) maxo=fabsf(l); if(fabsf(r)>maxo) maxo=fabsf(r);}
    printf("rt v%d K=%d ok, max|out|=%.3f missed=%lu\n", v+1, g_req_K, maxo, g_missed);
  }
  return 0;}
