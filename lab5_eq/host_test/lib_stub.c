/* host emulacija biquad(): pretpostavljen format a2,a1,b2,b1,b0, a-koef. negirani,
   direktna forma II, stanje 2*sections. Sluzi samo da se provjeri autodetekcija. */
float *biquad(const float in[], float out[], const float co[], float st[], int n, int sec)
{
    int i, k;
    for (i = 0; i < n; i++) {
        float x = in[i];
        for (k = 0; k < sec; k++) {
            const float *c = &co[5*k];
            float a2 = c[0], a1 = c[1], b2 = c[2], b1 = c[3], b0 = c[4];
            float w = x + a1 * st[2*k] + a2 * st[2*k+1];
            float y = b0 * w + b1 * st[2*k] + b2 * st[2*k+1];
            st[2*k+1] = st[2*k]; st[2*k] = w;
            x = y;
        }
        out[i] = x;
    }
    return out;
}
