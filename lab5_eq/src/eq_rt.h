/*
 * eq_rt.h - ugradnja ekvilajzera u postojeci talkthrough projekat (Lab 5).
 * SDOS 2026, Lab 5, tema 6 - Student: Luka Jovetic
 */
#ifndef EQ_RT_H
#define EQ_RT_H

/* Promjenljive koje se mijenjaju/citaju iz debagera (Expressions). */
extern volatile int g_req_version;            /* 0..4 = V1..V5                     */
extern volatile int g_req_K;                  /* broj sekcija (1..EQ_KMAX)          */
extern volatile int g_bypass;                 /* 1 = bez obrade (samo talkthrough) */
extern volatile unsigned long g_missed;       /* broj propustenih rokova           */
extern volatile unsigned long g_isr_count;    /* broj prekida                      */
extern volatile unsigned long g_cyc_eq_last, g_cyc_eq_max;   /* ciklusi EQ         */
extern volatile unsigned long g_cyc_isr_last, g_cyc_isr_max; /* ciklusi cijelog ISR */

void eq_rt_init(float fcclk_hz, float fs_hz);  /* poziv jednom, poslije inicijalizacije */
void eq_rt_poll(void);                         /* poziv u while(1) petlji u main-u       */
void eq_rt_isr_begin(void);                    /* prva naredba u SPORT ISR-u             */
void eq_rt_isr_end(void);                      /* zadnja naredba u SPORT ISR-u           */
void eq_rt_process(float *l, float *r);        /* obrada jednog stereo odmjerka [-1,1)   */
void eq_rt_process_int(int *l, int *r);        /* isto, za cjelobrojne odmjerke          */

#endif
