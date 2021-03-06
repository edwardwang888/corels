#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

void run(double r, char *outfile, char *dataset)
{
    char data_minor[100];
    sprintf(data_minor, "../data/%s.minor", dataset);
    // Start assembling command string
    char cmd[500];
    sprintf(cmd, "./corels -r %f -c 2 -p 1 -o %s ", r, outfile);
    if (strcmp(outfile, "falling.csv") == 0) {
        strcat(cmd, "-d ");
    }
    char temp[200];
    if (access(data_minor, F_OK) != 0)
        sprintf(temp, "../data/%1$s.out ../data/%1$s.label", dataset);
    else
        sprintf(temp, "../data/%1$s.out ../data/%1$s.label ../data/%1$s.minor", dataset);
    
    strcat(cmd, temp);
    printf("\n%s\n", cmd);
    system(cmd);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Missing argument.\n");
        exit(1);
    }
    
    int falling = 1;
    int opt;
    /* Specify -z for no falling */
    while((opt = getopt(argc, argv, "z")) != -1) {
        if (opt == 'z')
            falling = 0;
    }
    
    double r = 0.15;
    unlink("default.csv");
    if (falling)
        unlink("falling.csv");
    for (int i = 0; i < 70; i++) {
        run(r, "default.csv", argv[optind]);
        if (falling)
            run(r, "falling.csv", argv[optind]);
        r /= 1.25;
    }
    
    // Run with zero regularity
    run(0, "default.csv", argv[optind]);
    if (falling)
        run(0, "falling.csv", argv[optind]);
    
    system("cat default.csv | uniq > default1.csv");
    rename("default1.csv", "default.csv");
    if (falling) {
        system("cat falling.csv | uniq > falling1.csv");
        rename("falling1.csv", "falling.csv");
    }
}