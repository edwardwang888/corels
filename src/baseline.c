#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "wpa_objective.h"

int main(int argc, char *argv[])
{
    int fd = open(argv[1], O_WRONLY|O_CREAT|O_APPEND, S_IRUSR|S_IWUSR);
    int nsamples = (argc - 4)/2;
    double z[nsamples];
    int y[nsamples];
    int wpa_max = atoi(argv[2]);
    double c = atof(argv[3]);
    for (int i = 0; i < nsamples; i++) {
        z[i] = atof(argv[4+i]);
        y[i] = atoi(argv[4+nsamples+i]);
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

    double wpa = wpa_objective(z, y, wpa_max, nsamples);
    char output[200];
    sprintf(output, "%f %f\n", c, wpa);
    printf("%s", output);
    write(fd, output, strlen(output));
}