/*
 * eq.c - Stereo ekvilajzer: kaskada K biquad sekcija (DF-II transponovana)
 *
 *   y[n]  = b0 x[n] + s1[n-1]
 *   s1[n] = b1 x[n] - a1 y[n] + s2[n-1]
 *   s2[n] = b2 x[n] - a2 y[n]
 *
 * Koeficijenti: "peaking EQ" iz Audio EQ Cookbook (R. Bristow-Johnson).
 *
 * VAZNO: projekat se prevodi SA optimizacijom (-O). Verzija 1 je izdvojena
 * pomocu #pragma optimize_off, pa se prevodi bez optimizacije.
 *
 * SDOS 2026, Lab 5, tema 6 - Student: Luka Jovetic
 */
#include <math.h>
#include <stdio.h>
#include "eq.h"

#if defined(__ADSP21000__) || defined(__ADSPSHARC__)
  #include <filter.h>
#else
  /* host test: emulacija bibliotecke funkcije (vidi host_test/lib_stub.c) */
  float *biquad(const float input[], float output[], const float coeffs[],
                float state[], int samples, int sections);
#endif

const char * const eq_name[EQ_NVER] = {
    "V1 C -O0", "V2 C -O", "V3 biquad()", "V4 SIMD", "V5 SIMD+dm/pm"
};

/* ---------------------------------------------------------------------------
 * Podaci
 * ------------------------------------------------------------------------- */

/* Verzije 1 i 2: koeficijenti {b0,b1,b2,a1,a2} po sekciji, sve u DM. */
static float dm c_sc[5 * EQ_KMAX];
static float dm s_sc[2][2 * EQ_KMAX];          /* [kanal][s1,s2 po sekciji] */

/* Verzija 3: biblioteka trazi koeficijente u PM memoriji. */
static float pm c_lib[5 * EQ_KMAX];
static float dm s_lib[2][2 * EQ_KMAX + 2];
static int  g_lib_variant = 0;                 /* format koeficijenata (autodetekcija) */

/* Verzije 4 i 5 (SIMD): svaki koeficijent dupliran za L i R, tako da
 * jedan dvostruki (SIMD) pristup ucita par (L,R).
 * Raspored po sekciji: b0L b0R b1L b1R b2L b2R a1L a1R a2L a2R          */
#pragma align 2
static float dm c_simd_dm[10 * EQ_KMAX];       /* V4: koeficijenti u DM (isti blok kao stanja) */
#pragma align 2
static float pm c_simd_pm[10 * EQ_KMAX];       /* V5: koeficijenti u PM (drugi blok)           */
#pragma align 2
static float dm s_simd[4 * EQ_KMAX];           /* s1L s1R s2L s2R po sekciji                   */
#pragma align 2
static float dm x_simd[2];                     /* radni par (L,R)                              */

static int g_designK = 0;

/* ---------------------------------------------------------------------------
 * Verzija 1: C kod bez optimizacije
 * ------------------------------------------------------------------------- */
#pragma optimize_off
static void eq_v1(int K, float *l, float *r)
{
    int ch, k;
    float x, y;
    const float *c;
    float *s;

    for (ch = 0; ch < 2; ch++) {
        x = (ch == 0) ? *l : *r;
        c = c_sc;
        s = s_sc[ch];
        for (k = 0; k < K; k++) {
            y    = c[0] * x + s[0];
            s[0] = c[1] * x - c[3] * y + s[1];
            s[1] = c[2] * x - c[4] * y;
            x = y;
            c += 5;
            s += 2;
        }
        if (ch == 0) *l = x; else *r = x;
    }
}
#pragma optimize_as_cmd_line

/* ---------------------------------------------------------------------------
 * Verzija 2: isti C kod, sa optimizacijom (-O)
 * ------------------------------------------------------------------------- */
static void eq_v2(int K, float *l, float *r)
{
    int ch, k;
    float x, y;
    const float *c;
    float *s;

    for (ch = 0; ch < 2; ch++) {
        x = (ch == 0) ? *l : *r;
        c = c_sc;
        s = s_sc[ch];
        for (k = 0; k < K; k++) {
            y    = c[0] * x + s[0];
            s[0] = c[1] * x - c[3] * y + s[1];
            s[1] = c[2] * x - c[4] * y;
            x = y;
            c += 5;
            s += 2;
        }
        if (ch == 0) *l = x; else *r = x;
    }
}

/* ---------------------------------------------------------------------------
 * Verzija 3: biblioteka (filter.h), jedan poziv po kanalu i odmjerku
 * ------------------------------------------------------------------------- */
static void eq_v3(int K, float *l, float *r)
{
    float in, out;

    in = *l;
    biquad(&in, &out, c_lib, s_lib[0], 1, K);
    *l = out;

    in = *r;
    biquad(&in, &out, c_lib, s_lib[1], 1, K);
    *r = out;
}

/* ---------------------------------------------------------------------------
 * Verzija 4: SIMD - L i R kanal paralelno (PEx racuna L, PEy racuna R).
 * Unutrasnja petlja ima dvije nezavisne iteracije (kanale); SIMD_for
 * dozvoljava kompajleru da ih izvrsi istovremeno na PEx i PEy.
 * ------------------------------------------------------------------------- */
static void eq_v4(int K, float *l, float *r)
{
    int k, ch;
    const float dm *c = c_simd_dm;
    float dm *s = s_simd;

    x_simd[0] = *l;
    x_simd[1] = *r;
    for (k = 0; k < K; k++) {
#pragma SIMD_for
        for (ch = 0; ch < 2; ch++) {
            float y   = c[ch] * x_simd[ch] + s[ch];
            s[ch]     = c[2 + ch] * x_simd[ch] - c[6 + ch] * y + s[2 + ch];
            s[2 + ch] = c[4 + ch] * x_simd[ch] - c[8 + ch] * y;
            x_simd[ch] = y;
        }
        c += 10;
        s += 4;
    }
    *l = x_simd[0];
    *r = x_simd[1];
}

/* ---------------------------------------------------------------------------
 * Verzija 5: kao V4, ali koeficijenti su u PM memoriji (blok 2), a stanja
 * u DM memoriji (blok 1). Koeficijent se cita preko PM magistrale, a stanje
 * preko DM magistrale u ISTOM ciklusu.
 * ------------------------------------------------------------------------- */
static void eq_v5(int K, float *l, float *r)
{
    int k, ch;
    const float pm *c = c_simd_pm;
    float dm *s = s_simd;

    x_simd[0] = *l;
    x_simd[1] = *r;
    for (k = 0; k < K; k++) {
#pragma SIMD_for
        for (ch = 0; ch < 2; ch++) {
            float y   = c[ch] * x_simd[ch] + s[ch];
            s[ch]     = c[2 + ch] * x_simd[ch] - c[6 + ch] * y + s[2 + ch];
            s[2 + ch] = c[4 + ch] * x_simd[ch] - c[8 + ch] * y;
            x_simd[ch] = y;
        }
        c += 10;
        s += 4;
    }
    *l = x_simd[0];
    *r = x_simd[1];
}

typedef void (*eq_fn_t)(int, float *, float *);
static eq_fn_t const eq_tbl[EQ_NVER] = { eq_v1, eq_v2, eq_v3, eq_v4, eq_v5 };

void eq_process(int ver, int K, float *l, float *r)
{
    eq_tbl[ver](K, l, r);
}

/* ---------------------------------------------------------------------------
 * Koeficijenti za biblioteku. Redoslijed i znak a-koeficijenata provjeriti u
 * CCES Help (C Runtime Library -> biquad); eq_lib_autodetect() isprobava 8
 * mogucih formata i bira onaj koji daje isti izlaz kao referenca.
 * ------------------------------------------------------------------------- */
static void fill_lib(int K)
{
    int k, order = g_lib_variant & 3, neg = (g_lib_variant >> 2) & 1;
    for (k = 0; k < K; k++) {
        float b0 = c_sc[5*k], b1 = c_sc[5*k+1], b2 = c_sc[5*k+2];
        float a1 = c_sc[5*k+3], a2 = c_sc[5*k+4];
        float pm *d = &c_lib[5*k];
        if (neg) { a1 = -a1; a2 = -a2; }
        switch (order) {
        case 0: d[0]=a2; d[1]=a1; d[2]=b2; d[3]=b1; d[4]=b0; break;
        case 1: d[0]=b0; d[1]=b1; d[2]=b2; d[3]=a1; d[4]=a2; break;
        case 2: d[0]=b2; d[1]=b1; d[2]=b0; d[3]=a2; d[4]=a1; break;
        default:d[0]=a1; d[1]=a2; d[2]=b0; d[3]=b1; d[4]=b2; break;
        }
    }
}

/* ---------------------------------------------------------------------------
 * Projektovanje: K peaking sekcija, logaritamski od 40 Hz do 16 kHz,
 * pojacanja po krivoj "osmijeh" (+6 dB na krajevima, -6 dB u sredini).
 * Za veliko K pojacanje i sirina opsega se smanjuju da ukupni odziv ostane
 * umjeren (bezbjedno za slusalice).
 * ------------------------------------------------------------------------- */
void eq_design(int K, float fs)
{
    int k;
    const float PI = 3.14159265f;
    float gmax = (K <= 12) ? 6.0f : 6.0f * 12.0f / (float)K;
    float Q    = (K <= 10) ? 1.41f : 1.41f * (float)(K - 1) / 9.0f;

    if (K < 1) K = 1;
    if (K > EQ_KMAX) K = EQ_KMAX;

    for (k = 0; k < K; k++) {
        float t  = (K == 1) ? 0.5f : (float)k / (float)(K - 1);
        float f0 = 40.0f * (float)pow(16000.0f / 40.0f, t);
        float G  = gmax * (float)cos(2.0f * PI * t);
        float A  = (float)pow(10.0f, G / 40.0f);
        float w0 = 2.0f * PI * f0 / fs;
        float cw = (float)cos(w0);
        float al = (float)sin(w0) / (2.0f * Q);
        float a0 = 1.0f + al / A;
        float b0 = (1.0f + al * A) / a0;
        float b1 = (-2.0f * cw) / a0;
        float b2 = (1.0f - al * A) / a0;
        float a1 = (-2.0f * cw) / a0;
        float a2 = (1.0f - al / A) / a0;
        float co[5];
        int i;

        co[0] = b0; co[1] = b1; co[2] = b2; co[3] = a1; co[4] = a2;
        for (i = 0; i < 5; i++) {
            c_sc[5*k + i]            = co[i];
            c_simd_dm[10*k + 2*i]     = co[i];
            c_simd_dm[10*k + 2*i + 1] = co[i];
            c_simd_pm[10*k + 2*i]     = co[i];
            c_simd_pm[10*k + 2*i + 1] = co[i];
        }
    }
    g_designK = K;
    fill_lib(K);
}

void eq_reset(void)
{
    int i;
    for (i = 0; i < 2 * EQ_KMAX; i++) { s_sc[0][i] = 0.0f; s_sc[1][i] = 0.0f; }
    for (i = 0; i < 2 * EQ_KMAX + 2; i++) { s_lib[0][i] = 0.0f; s_lib[1][i] = 0.0f; }
    for (i = 0; i < 4 * EQ_KMAX; i++) s_simd[i] = 0.0f;
}

/* ---------------------------------------------------------------------------
 * Provjera tacnosti: poredjenje sa referentnom implementacijom u long double.
 * Ulaz je pseudoslucajni sum; L i R imaju razlicite ulaze.
 * ------------------------------------------------------------------------- */
static unsigned long lcg_state;
static float noise(void)
{
    lcg_state = lcg_state * 1664525UL + 1013904223UL;
    return (float)((lcg_state >> 8) & 0xFFFF) / 65536.0f - 0.5f;
}

float eq_check(int ver, int K, int n)
{
    long double rs[2][2 * EQ_CHECK_KMAX];
    float maxerr = 0.0f;
    int i, k, ch;

    if (K > EQ_CHECK_KMAX) K = EQ_CHECK_KMAX;
    if (K > g_designK) K = g_designK;
    for (ch = 0; ch < 2; ch++)
        for (i = 0; i < 2 * EQ_CHECK_KMAX; i++) rs[ch][i] = 0.0L;
    eq_reset();
    lcg_state = 12345UL;

    for (i = 0; i < n; i++) {
        float l = noise(), r = noise();
        long double xr[2];
        xr[0] = l; xr[1] = r;
        for (ch = 0; ch < 2; ch++) {
            long double x = xr[ch], y;
            for (k = 0; k < K; k++) {
                const float *c = &c_sc[5 * k];
                y = c[0] * x + rs[ch][2*k];
                rs[ch][2*k]   = c[1] * x - c[3] * y + rs[ch][2*k + 1];
                rs[ch][2*k+1] = c[2] * x - c[4] * y;
                x = y;
            }
            xr[ch] = x;
        }
        eq_process(ver, K, &l, &r);
        {
            float e1 = (float)fabs((double)(l - (float)xr[0]));
            float e2 = (float)fabs((double)(r - (float)xr[1]));
            if (!(e1 == e1)) e1 = 1e9f;   /* NaN */
            if (!(e2 == e2)) e2 = 1e9f;
            if (e1 > maxerr) maxerr = e1;
            if (e2 > maxerr) maxerr = e2;
        }
    }
    eq_reset();
    return maxerr;
}

int eq_lib_autodetect(void)
{
    int v, best = -1;
    float besterr = 1e30f;

    /* Test na K = 3. Poslije poziva treba ponovo pozvati eq_design(K, fs). */
    eq_design(3, 48000.0f);
    for (v = 0; v < 8; v++) {
        float e;
        g_lib_variant = v;
        fill_lib(g_designK);
        e = eq_check(EQ_V3_LIB, g_designK, 128);
        if (e < besterr) { besterr = e; best = v; }
    }
    g_lib_variant = (best < 0) ? 0 : best;
    fill_lib(g_designK);
    return (besterr < 1e-3f) ? best : -1;
}

void eq_print_addresses(void)
{
    printf("Adrese nizova (blok 1: 0xB2000-0xBDFFF, blok 2: 0xC0000-0xC7FFF, blok 3: 0xE0000-0xE7FFF):\n");
    printf("  c_sc      (V1,V2 koef.)   = 0x%08lX\n", (unsigned long)c_sc);
    printf("  s_sc      (V1,V2 stanja)  = 0x%08lX\n", (unsigned long)s_sc);
    printf("  c_lib     (V3 koef., pm)  = 0x%08lX\n", (unsigned long)c_lib);
    printf("  s_lib     (V3 stanja)     = 0x%08lX\n", (unsigned long)s_lib);
    printf("  c_simd_dm (V4 koef., dm)  = 0x%08lX\n", (unsigned long)c_simd_dm);
    printf("  c_simd_pm (V5 koef., pm)  = 0x%08lX\n", (unsigned long)c_simd_pm);
    printf("  s_simd    (V4,V5 stanja)  = 0x%08lX\n", (unsigned long)s_simd);
}
