#include "rule.h"

void update_scores(double *scores, int nsamples, VECTOR captured, int wpa, int ruleindex, double proportion)
{
    for (int i = 0; i < nsamples; i++) {
        if (rule_isset(captured, nsamples-i-1)) {
            if (wpa)
                *(scores+i) = -1 * ruleindex;
            else
                *(scores+i) = proportion;
        }
    }
}

double a(double base, int pow)
{
    double value = 1;
    for (int i = 0; i < pow; i++)
        value *= base;
    return value;
}

double wpa_objective(double *z, int *y, unsigned long long wpa_max, int nsamples, int e)
{
    fprintf(stderr, "Calling wpa_objective\n");
    double wpa = 0;
    for (int i = 0; i < nsamples; i++) {
        for (int j = 0; j < nsamples; j++) {
            wpa += a(((z[i] >= z[j]) - 0.5 * (z[i] == z[j])) * (y[i] > y[j]), e);
            // if (z[i] < z[j] && y[i] < y[j])
            //     wpa -= 1;
        }
    }
    //printf("%f %d %f %f\n", wpa, wpa_max, wpa/wpa_max, wpa/wpa_max + 1);
    // wpa = wpa/wpa_max + 1;
    wpa = wpa/wpa_max;
    return wpa;
}