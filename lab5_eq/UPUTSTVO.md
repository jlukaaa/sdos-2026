# SDOS Lab 5 - tema 6: Stereo ekvilajzer na SHARC-u (Luka Jovetić)

Sadržaj paketa:

| fajl | namjena |
|---|---|
| `src/eq.h`, `src/eq.c` | ekvilajzer: 5 verzija + projektovanje koeficijenata |
| `src/eq_bench.c` | mjerenje ciklusa (vlastiti `main`, radi i na simulatoru) |
| `src/eq_rt.h`, `src/eq_rt.c` | ugradnja u talkthrough projekat + brojač propuštenih rokova |
| `tools/plot_results.py` | grafici iz CSV-a koji ispiše benchmark |
| `host_test/` | testovi logike na računaru (gcc), nije za CCES |

**Putanja projekta ne smije sadržavati Č, Ć, Š, Đ, Ž** (npr. `C:\SDOS\...`).

## Korak 1 - mjerenje ciklusa (projekat `eq_bench`)

1. File → New → CrossCore Project, ime `eq_bench`, ADSP-21489, rev. 0.2, add-in Startup Code/LDF.
2. Obriši generisani `src/eq_bench.c` (ako postoji) i ubaci `eq.h`, `eq.c`, `eq_bench.c` u `src`
   (desni klik na `src` → Import → File System, ili prevuci fajlove).
   Ako je generisan drugi `.c` sa `main`-om, obriši ga – smije postojati samo jedan `main`.
3. Project → Properties → C/C++ Build → Settings → Compiler → General:
   **Enable optimization (-O) = uključeno**, kriterijum 100 (kao Lab 3, Primjer 5).
   Verzija 1 se sama prevodi bez optimizacije (`#pragma optimize_off`).
4. U `eq_bench.c` provjeri `FCCLK_HZ` (takt iz `init_PLL_SDRAM.c` iz Lab 5).
5. Build → Debug (simulator ili ploča) → Resume.
6. Cijeli ispis iz Console kopiraj u `log_bench.txt` (ide uz izvještaj).
   Redove između `CSV_BEGIN` i `CSV_END` kopiraj u `results.csv`.
7. `python tools/plot_results.py results.csv` → `slika1_ciklusi_vs_K.png`, `slika2_ciklusi_po_sekciji.png`.

Provjere u logu:
- sve verzije u "Provjera tačnosti" moraju imati **OK**;
- linija `biquad(): format koeficijenata = varijanta ...` – ako piše UPOZORENJE, V3 nije validna
  (pogledati CCES Help → biquad i javiti);
- adrese: `c_simd_pm` mora biti u bloku 2 (0xC....), `s_simd` u bloku 1 ili 3 (0xB.... / 0xE....).
- ako V4 nema manje ciklusa od V2, kompajler nije vektorizovao petlju – pogledati Disassembly
  (traži se `PEYEN` / SIMD instrukcije) i to navesti u analizi.

## Korak 2 - ekvilajzer u realnom vremenu (talkthrough projekat iz Lab 5)

1. U projekat Lab 5 dodaj `eq.h`, `eq.c`, `eq_rt.h`, `eq_rt.c` (**ne** `eq_bench.c`). Uključi -O.
2. U `main` (poslije inicijalizacije PLL/SPORT/kodeka):
   ```c
   #include "eq_rt.h"
   ...
   eq_rt_init(400000000.0f, 48000.0f);   /* fCCLK i fs iz projekta */
   while (1) {
       eq_rt_poll();
   }
   ```
3. U SPORT ISR-u (prekidna rutina koja prima/šalje odmjerke):
   ```c
   eq_rt_isr_begin();                   /* prva naredba u ISR-u            */
   ... postojeći kod čita L i R ...
   eq_rt_process(&L, &R);               /* ako su L, R float u [-1, 1)      */
   /* ili eq_rt_process_int(&L, &R);    ako su L, R int (vidi SAMPLE_FULL_SCALE) */
   ... postojeći kod šalje L i R ...
   eq_rt_isr_end();                     /* zadnja naredba u ISR-u          */
   ```
4. **Prvo smanji jačinu na slušalicama.** Pokreni, posluša se razlika uz `g_bypass = 1 / 0`.
5. Expressions: dodaj `g_req_version`, `g_req_K`, `g_bypass`, `g_missed`, `g_isr_count`,
   `g_cyc_eq_max`, `g_cyc_isr_max`.

### Određivanje maksimalnog K (brojač propuštenih rokova)

Za svaku verziju (0..4):
1. Pause → upiši `g_req_version` i `g_req_K` → Resume (main petlja preprojektuje filtar i
   obriše `g_missed`).
2. Sačekaj ~5 s → Pause → pročitaj `g_missed` i `g_isr_count`.
3. `g_missed = 0` → povećaj K; `g_missed > 0` → smanji K (polovljenje intervala).
4. Najveći K sa `g_missed = 0` upisati u tabelu. Uz to zapisati `g_cyc_eq_max` i `g_cyc_isr_max`.

Cijena prekida bez EQ (`ISR_OVERHEAD`): `g_bypass = 1` → `g_cyc_isr_max`. Taj broj upisati u
`eq_bench.c` i u izvještaj (koristi se u predviđanju K_max).
Ako je za V5 predviđeni K_max veći od `EQ_KMAX = 1024`, upisati "> 1024 (ograničeno memorijom)".

## Korak 3 - izvještaj

U `Izvjestaj_Lab5_Tema6_Luka_Jovetic.docx` sva žuto označena polja `___` popuniti izmjerenim
vrijednostima, ubaciti dvije slike i dopuniti analizu.
