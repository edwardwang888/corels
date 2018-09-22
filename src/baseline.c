#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "wpa_objective.h"

int main(int argc, char *argv[])
{
    int e = 1;
    unsigned long long wpa_max = 0;
    int c;
    while ((c = getopt(argc, argv, "e:m:")) != -1) {
        switch (c) {
            case 'e':
                e = atoi(optarg);
                break;
            case 'm':
                wpa_max = atoll(optarg);
                fprintf(stderr, "wpa_max: %llu\n", wpa_max);
                break;
        }
    }
    if (wpa_max == 0)
        fprintf(stderr, "wpa_max not set\n");

    int nsamples = (argc - optind)/2;
    double z[nsamples];
    int y[nsamples];
    for (int i = 0; i < nsamples; i++) {
        z[i] = atof(argv[optind+i]);
        y[i] = atoi(argv[optind+nsamples+i]);
    }

/*  
    printf("%d %d %f\n", nsamples, wpa, c);
    for (int i = 0; i < nsamples; i++)
        printf("%f ", z[i]);
    printf("\n");
    for (int i = 0; i < nsamples; i++)
        printf("%d ", y[i]);
 */
/* 
    double wpa = 0;
    for (int i = 0; i < nsamples; i++) {
        for (int j = 0; j < i; j++) {
            wpa -= (((z[i] >= z[j]) - 0.5 * (z[i] == z[j])) * (y[i] > y[j]));
            if (z[i] < z[j] && y[i] < y[j])
                wpa -= 1;
        }
    }
    wpa = wpa/wpa_max + 1;
 */

    double wpa = wpa_objective(z, y, wpa_max, nsamples, e);
    printf("%1.40f", wpa);
}