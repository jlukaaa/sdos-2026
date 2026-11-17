/*
 * eq.h - Stereo ekvilajzer (kaskada K biquad sekcija), 5 verzija
 * SDOS 2026, Lab 5, tema 6 - Arhitektura SHARC procesora
 * Student: Luka Jovetic
 */
#ifndef EQ_H
#define EQ_H

/* Na racunaru (host test) kvalifikatori dm/pm ne postoje. */
#if !defined(__ADSP21000__) && !defined(__ADSPSHARC__)
  #define dm
  #define pm
#endif

/* Maksimalan broj sekcija (ogranicen internom memorijom). */
#define EQ_KMAX        1024
/* Maksimalan K za koji se radi provjera tacnosti (eq_check). */
#define EQ_CHECK_KMAX  16

enum {
    EQ_V1_O0 = 0,   /* 1. C kod bez optimizacije (#pragma optimize_off)     */
    EQ_V2_O,        /* 2. isti C kod sa optimizacijom (-O)                  */
    EQ_V3_LIB,      /* 3. biblioteka: biquad() iz filter.h                  */
    EQ_V4_SIMD,     /* 4. SIMD: L i R paralelno na PEx/PEy                  */
    EQ_V5_DMPM,     /* 5. SIMD + koeficijenti u PM bloku, stanja u DM bloku */
    EQ_NVER
};

extern const char * const eq_name[EQ_NVER];

void  eq_design(int K, float fs);              /* racuna koeficijente za sve verzije   */
void  eq_reset(void);                          /* brise stanja svih verzija            */
void  eq_process(int ver, int K, float *l, float *r); /* jedan stereo odmjerak, in-place */
int   eq_lib_autodetect(void);                 /* pronalazi format koeficijenata za biquad() */
float eq_check(int ver, int K, int n);         /* max |greska| u odnosu na referencu   */
void  eq_print_addresses(void);                /* ispis adresa nizova (memorijski blok) */

#endif
