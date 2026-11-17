#include <stdio.h>
#include <math.h>
#include "../src/eq.h"
static double gain_db(int K, float f){ int i; double pin=0,pout=0;
  eq_design(K,48000.f); eq_reset();
  for(i=0;i<48000*2;i++){ float x=0.25f*sinf(2*3.14159265f*f*i/48000.f), l=x, r=x;
    eq_process(4,K,&l,&r); if(i>48000){pin+=x*x; pout+=l*l;} if(!isfinite(l)) return 999;}
  return 10*log10(pout/pin);}
int main(void){ int Ks[]={3,8,10,64,1024}; float fs[]={40,100,500,1500,5000,16000}; int a,b;
  for(a=0;a<5;a++){ printf("K=%4d:",Ks[a]); for(b=0;b<6;b++) printf(" %6.0fHz:%+6.2fdB",fs[b],gain_db(Ks[a],fs[b])); printf("\n");}
  return 0;}
