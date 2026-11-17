"""
plot_results.py - grafici iz rezultata benchmarka (eq_bench.c).

Upotreba:
  1. Iz Console ispisa kopirati redove izmedju CSV_BEGIN i CSV_END u results.csv
     (zajedno sa zaglavljem version,K,...).
  2. python plot_results.py results.csv
  -> slika1_ciklusi_vs_K.png, slika2_ciklusi_po_sekciji.png

Potrebno: pip install matplotlib
SDOS 2026, Lab 5, tema 6 - Student: Luka Jovetic
"""
import csv
import math
import sys
import matplotlib.pyplot as plt

NAMES = {1: "V1 C -O0", 2: "V2 C -O", 3: "V3 biquad()", 4: "V4 SIMD", 5: "V5 SIMD+dm/pm"}
COLORS = {1: "#2a78d6", 2: "#eb6834", 3: "#1baf7a", 4: "#eda100", 5: "#e87ba4"}
MARKERS = {1: "o", 2: "s", 3: "^", 4: "D", 5: "v"}
INK, MUTED, GRID = "#1f1f1e", "#6b6b66", "#e4e3df"


def load(path):
    data = {}
    with open(path, newline="") as f:
        for row in csv.DictReader(f):
            v = int(row["version"])
            data.setdefault(v, []).append((int(row["K"]), float(row["C_min"])))
    for v in data:
        data[v].sort()
    return data


def fit(points):
    n = len(points)
    sx = sum(k for k, _ in points); sy = sum(c for _, c in points)
    sxx = sum(k * k for k, _ in points); sxy = sum(k * c for k, c in points)
    b = (n * sxy - sx * sy) / (n * sxx - sx * sx)
    return (sy - b * sx) / n, b


def style(ax):
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    for s in ("left", "bottom"):
        ax.spines[s].set_color(MUTED)
    ax.tick_params(colors=MUTED, labelsize=9)
    ax.grid(True, color=GRID, linewidth=0.8)
    ax.set_axisbelow(True)


def main(path):
    data = load(path)

    # Slika 1: ciklusi po stereo odmjerku u zavisnosti od K (log skala)
    fig, ax = plt.subplots(figsize=(7.5, 4.6), dpi=150)
    ends = []
    for v, pts in sorted(data.items()):
        ks = [k for k, _ in pts]; cs = [max(c, 1) for _, c in pts]
        ax.plot(ks, cs, color=COLORS[v], linewidth=2, marker=MARKERS[v], markersize=6,
                markeredgecolor="white", markeredgewidth=1, label=NAMES[v])
        ends.append([math.log10(cs[-1]), v, ks[-1], cs[-1]])
    # razmakni natpise na desnom kraju da se ne preklapaju (min. razmak u dekadama)
    ends.sort()
    gap = 0.08
    for i in range(1, len(ends)):
        if ends[i][0] - ends[i - 1][0] < gap:
            ends[i][0] = ends[i - 1][0] + gap
    kmax = max(e[2] for e in ends)
    for ly, v, kx, cy in ends:
        ax.text(kx * 1.12, 10 ** ly, NAMES[v], va="center", fontsize=8, color=INK)
    ax.set_xscale("log", base=2); ax.set_yscale("log")
    ax.set_xlim(0.8, kmax * 3.2)
    ax.set_xlabel("broj sekcija K", color=INK); ax.set_ylabel("ciklusa po stereo odmjerku", color=INK)
    ax.set_title("Slika 1. Cijena obrade jednog stereo odmjerka", color=INK, fontsize=11, loc="left")
    style(ax); ax.legend(frameon=False, fontsize=8, loc="upper left")
    fig.tight_layout(); fig.savefig("slika1_ciklusi_vs_K.png"); plt.close(fig)

    # Slika 2: nagib b/2 = ciklusa po sekciji po kanalu, uz teorijske minimume
    fig, ax = plt.subplots(figsize=(7.5, 4.2), dpi=150)
    vs = sorted(data); vals = [fit(data[v])[1] / 2.0 for v in vs]
    bars = ax.bar([NAMES[v] for v in vs], vals, color=[COLORS[v] for v in vs], width=0.6,
                  edgecolor="white", linewidth=2)
    for b_, val in zip(bars, vals):
        ax.annotate(f"{val:.2f}", (b_.get_x() + b_.get_width() / 2, val), xytext=(0, 3),
                    textcoords="offset points", ha="center", fontsize=9, color=INK)
    ax.axhline(5.0, color=MUTED, linestyle="--", linewidth=1, label="skalarni minimum = 5")
    ax.axhline(2.5, color=MUTED, linestyle=":", linewidth=1.5, label="SIMD minimum = 2,5")
    ax.legend(frameon=False, fontsize=8, loc="upper right")
    ax.set_ylabel("ciklusa po sekciji po kanalu", color=INK)
    ax.set_title("Slika 2. Izmjerena cijena sekcije (nagib b/2) i teorijski minimum",
                 color=INK, fontsize=11, loc="left")
    style(ax); ax.grid(axis="x", visible=False)
    fig.tight_layout(); fig.savefig("slika2_ciklusi_po_sekciji.png"); plt.close(fig)

    print("Model C(K) = a + b*K:")
    for v in vs:
        a, b = fit(data[v])
        print(f"  {NAMES[v]:14s} a = {a:8.1f}  b = {b:7.2f}  b/2 = {b/2:6.2f}  "
              f"eff(5) = {500/(b/2):6.1f} %  eff(2,5) = {250/(b/2):6.1f} %")


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "results.csv")
