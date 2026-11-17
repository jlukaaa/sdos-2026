/* host stub: brojac je obicni brojac poziva (samo za provjeru logike) */
typedef unsigned long cycle_t;
static unsigned long __fake_clk;
#define START_CYCLE_COUNT(S) ((S) = (__fake_clk += 7))
#define STOP_CYCLE_COUNT(C, S) ((C) = (__fake_clk += 7) - (S))
