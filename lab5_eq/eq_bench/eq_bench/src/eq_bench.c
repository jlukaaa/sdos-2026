/*
 * eq_bench.c - mjerenje broja ciklusa za svih 5 verzija ekvilajzera.
 * Radi i na simulatoru i na ploci (ne koristi audio).
 *
 * Projekat prevesti SA optimizacijom (-O), kao u Lab 3, Primjer 5.
 * Ispis iz Console sacuvati kao log (log_bench.txt); CSV dio na kraju
 * sacuvati kao results.csv za grafike (plot_results.py).
 *
 * SDOS 2026, Lab 5, tema 6 - Student: Luka Jovetic
 */
#include <stdio.h>
#define DO_CYCLE_COUNTS
#include <cycle_count.h>
#include "eq.h"

/* ---- PODESITI prema svom projektu ---------------------------------------- */
#define FCCLK_HZ      400000000.0f  /* takt jezgra (provjeriti init_PLL_SDRAM.c) */
#define ISR_OVERHEAD  200.0f        /* ciklusi prekida bez EQ (izmjeriti u eq_rt) */
/* --------------------------------------------------------------------------- */

#define FS_DESIGN     48000.0f
#define NREP          16

static const int Ks[] = { 1, 2, 3, 4, 6, 8, 10, 12, 16, 32, 64 };
#define NK ((int)(sizeof(Ks) / sizeof(Ks[0])))

static long cmin_tab[EQ_NVER][NK];
static long cavg_tab[EQ_NVER][NK];

static unsigned long lcg = 1u;
static float rnd(void)
{
    lcg = lcg * 1664525UL + 1013904223UL;
    return (float)((lcg >> 8) & 0xFFFF) / 65536.0f - 0.5f;
}

int main(void)
{
    int v, i, rep, lib;
    long ovh = 1000000L;
    cycle_t t0, t1;
    const float budget48 = FCCLK_HZ / 48000.0f;
    const float budget96 = FCCLK_HZ / 96000.0f;

    printf("=== SDOS Lab5 / tema 6: stereo EQ, mjerenje ciklusa ===\n");
    printf("fCCLK = %.0f Hz, budzet: %.0f cikl. (48 kHz), %.0f cikl. (96 kHz)\n",
           FCCLK_HZ, budget48, budget96);

    /* 1) Format koeficijenata bibliotecke funkcije biquad() */
    lib = eq_lib_autodetect();
    if (lib < 0)
        printf("UPOZORENJE: biquad() ne daje isti izlaz kao referenca - V3 nije validna!\n");
    else
        printf("biquad(): format koeficijenata = varijanta %d (red %d, a-koef. %s)\n",
               lib, lib & 3, (lib & 4) ? "negirani" : "kao u formuli");

    /* 2) Provjera tacnosti svih verzija (K = 8, 256 odmjeraka) */
    eq_design(8, FS_DESIGN);
    printf("\nProvjera tacnosti (K=8, 256 odmjeraka, max |greska| vs. referenca):\n");
    for (v = 0; v < EQ_NVER; v++) {
        float e = eq_check(v, 8, 256);
        printf("  %-14s  %.3e  %s\n", eq_name[v], e, (e < 1e-4f) ? "OK" : "GRESKA");
    }
    eq_print_addresses();

    /* 3) Kalibracija: cijena samog mjerenja (prazno mjerenje) */
    for (rep = 0; rep < 16; rep++) {
        long c;
        START_CYCLE_COUNT(t0);
        STOP_CYCLE_COUNT(t1, t0);
        c = (long)t1;
        if (c < ovh) ovh = c;
    }
    printf("\nCijena mjerenja (oduzima se): %ld ciklusa\n", ovh);

    /* 4) Mjerenje: C(K) = ciklusi za jedan STEREO odmjerak */
    printf("\n%-14s %4s %10s %10s %14s\n", "verzija", "K", "C_min", "C_avg", "C/(2K)");
    for (v = 0; v < EQ_NVER; v++) {
        for (i = 0; i < NK; i++) {
            int K = Ks[i];
            long cmin = 1000000000L, csum = 0;
            eq_design(K, FS_DESIGN);
            eq_reset();
            for (rep = 0; rep < NREP; rep++) {
                float l = rnd(), r = rnd();
                long c;
                START_CYCLE_COUNT(t0);
                eq_process(v, K, &l, &r);
                STOP_CYCLE_COUNT(t1, t0);
                c = (long)t1 - ovh;
                csum += c;
                if (c < cmin) cmin = c;
            }
            cmin_tab[v][i] = cmin;
            cavg_tab[v][i] = csum / NREP;
            printf("%-14s %4d %10ld %10ld %14.2f\n", eq_name[v], K, cmin,
                   csum / NREP, (float)cmin / (2.0f * (float)K));
        }
    }

    /* 5) Linearni model C(K) = a + b*K (najmanji kvadrati), efikasnost, Kmax */
    printf("\nModel C(K) = a + b*K  (b/2 = ciklusa po sekciji po kanalu)\n");
    printf("%-14s %9s %9s %9s %10s %10s %8s %8s\n",
           "verzija", "a", "b", "b/2", "eff(5)%", "eff(2.5)%", "Kmax48", "Kmax96");
    for (v = 0; v < EQ_NVER; v++) {
        float sx = 0, sy = 0, sxx = 0, sxy = 0, n = (float)NK, a, b, bch, k48, k96;
        for (i = 0; i < NK; i++) {
            float x = (float)Ks[i], y = (float)cmin_tab[v][i];
            sx += x; sy += y; sxx += x * x; sxy += x * y;
        }
        b = (n * sxy - sx * sy) / (n * sxx - sx * sx);
        a = (sy - b * sx) / n;
        bch = b / 2.0f;
        k48 = (budget48 - ISR_OVERHEAD - a) / b;
        k96 = (budget96 - ISR_OVERHEAD - a) / b;
        printf("%-14s %9.1f %9.2f %9.2f %10.1f %10.1f %8d %8d\n", eq_name[v], a, b, bch,
               100.0f * 5.0f / bch, 100.0f * 2.5f / bch, (int)k48, (int)k96);
    }

    /* 6) CSV za grafike */
    printf("\nCSV_BEGIN\nversion,K,C_min,C_avg,per_section_per_channel\n");
    for (v = 0; v < EQ_NVER; v++)
        for (i = 0; i < NK; i++)
            printf("%d,%d,%ld,%ld,%.3f\n", v + 1, Ks[i], cmin_tab[v][i], cavg_tab[v][i],
                   (float)cmin_tab[v][i] / (2.0f * (float)Ks[i]));
    printf("CSV_END\nGotovo.\n");
    return 0;
}
