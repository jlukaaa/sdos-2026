/*
 * eq_rt.c - ekvilajzer u realnom vremenu + brojac propustenih rokova.
 *
 * Rok: obrada jednog odmjerka mora se zavrsiti prije sljedeceg prekida,
 * tj. u B = fCCLK / fs ciklusa. Ako je razmak izmedju dva ulaska u ISR
 * veci od 1,5*B, prekid je zakasnio -> propusten rok (g_missed++).
 *
 * SDOS 2026, Lab 5, tema 6 - Student: Luka Jovetic
 */
#define DO_CYCLE_COUNTS
#include <cycle_count.h>
#include "eq.h"
#include "eq_rt.h"

/* Puna skala cjelobrojnog odmjerka u eq_rt_process_int():
 * 2147483648.0f ako je 24-bitni odmjerak poravnat ulijevo u 32-bitnoj rijeci,
 * 8388608.0f ako je poravnat udesno (provjeriti u ISR-u talkthrough primjera). */
#define SAMPLE_FULL_SCALE  2147483648.0f
#define PRE_GAIN           0.5f     /* -6 dB rezerva, EQ ima do +6 dB pojacanja */

volatile int g_req_version = EQ_V5_DMPM;
volatile int g_req_K = 8;
volatile int g_bypass = 0;
volatile unsigned long g_missed = 0, g_isr_count = 0;
volatile unsigned long g_cyc_eq_last = 0, g_cyc_eq_max = 0;
volatile unsigned long g_cyc_isr_last = 0, g_cyc_isr_max = 0;

static volatile int ready = 0;
static int cur_ver = -1, cur_K = -1;
static float g_fs = 48000.0f;
static unsigned long budget = 8333;
static cycle_t t_prev, t_isr;
static int have_prev = 0;

void eq_rt_init(float fcclk_hz, float fs_hz)
{
    g_fs = fs_hz;
    budget = (unsigned long)(fcclk_hz / fs_hz);
    eq_lib_autodetect();      /* format koeficijenata za V3 */
    cur_K = -1;
    eq_rt_poll();
}

/* Promjena verzije ili K se radi u main petlji (projektovanje koristi sin/cos
 * i predugo traje za ISR). Za to vrijeme ISR propusta signal bez obrade. */
void eq_rt_poll(void)
{
    if (g_req_K != cur_K || g_req_version != cur_ver) {
        int K = g_req_K, v = g_req_version;
        if (K < 1) K = 1;
        if (K > EQ_KMAX) K = EQ_KMAX;
        if (v < 0 || v >= EQ_NVER) v = EQ_V5_DMPM;
        ready = 0;
        eq_design(K, g_fs);
        eq_reset();
        cur_K = K; cur_ver = v;
        g_req_K = K; g_req_version = v;
        g_missed = 0; g_cyc_eq_max = 0; g_cyc_isr_max = 0;
        have_prev = 0;
        ready = 1;
    }
}

void eq_rt_isr_begin(void)
{
    cycle_t now;
    START_CYCLE_COUNT(now);
    t_isr = now;
    g_isr_count++;
    if (have_prev && (unsigned long)(now - t_prev) > budget + budget / 2)
        g_missed++;
    t_prev = now;
    have_prev = 1;
}

void eq_rt_isr_end(void)
{
    cycle_t d;
    STOP_CYCLE_COUNT(d, t_isr);
    g_cyc_isr_last = (unsigned long)d;
    if (g_cyc_isr_last > g_cyc_isr_max) g_cyc_isr_max = g_cyc_isr_last;
}

static float guard(float x)
{
    if (!(x == x)) return 0.0f;             /* NaN */
    if (x >  0.999999f) return  0.999999f;
    if (x < -1.0f)      return -1.0f;
    return x;
}

void eq_rt_process(float *l, float *r)
{
    cycle_t t0, d;
    float L, R;

    if (!ready || g_bypass) return;
    L = *l * PRE_GAIN;
    R = *r * PRE_GAIN;
    START_CYCLE_COUNT(t0);
    eq_process(cur_ver, cur_K, &L, &R);
    STOP_CYCLE_COUNT(d, t0);
    g_cyc_eq_last = (unsigned long)d;
    if (g_cyc_eq_last > g_cyc_eq_max) g_cyc_eq_max = g_cyc_eq_last;
    *l = guard(L);
    *r = guard(R);
}

void eq_rt_process_int(int *l, int *r)
{
    float L = (float)(*l) / SAMPLE_FULL_SCALE;
    float R = (float)(*r) / SAMPLE_FULL_SCALE;
    eq_rt_process(&L, &R);
    *l = (int)(L * SAMPLE_FULL_SCALE);
    *r = (int)(R * SAMPLE_FULL_SCALE);
}
