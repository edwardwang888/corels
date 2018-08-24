double wpa_objective(double *z, int *y, int wpa_max, int nsamples)
{
    double wpa = 0;
    for (int i = 0; i < nsamples; i++) {
        for (int j = 0; j < nsamples; j++) {
            wpa += ((z[i] >= z[j]) - 0.5 * (z[i] == z[j])) * (y[i] > y[j]);
            // if (z[i] < z[j] && y[i] < y[j])
            //     wpa -= 1;
        }
    }
    //printf("%f %d %f %f\n", wpa, wpa_max, wpa/wpa_max, wpa/wpa_max + 1);
    //wpa = wpa/wpa_max + 1;
    wpa = wpa/wpa_max;
    return wpa;
}