#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

int main()
{
    double r = 0.15;
    unlink("default.csv");
    unlink("falling.csv");
    for (int i = 0; i < 70; i++) {
        char cmd[200];
        sprintf(cmd, "./corels -r %f -c 2 -p 1 -o %s ../data/compas_train.out ../data/compas_train.label ../data/compas_train.minor", r, "default.csv");
        system(cmd);
        sprintf(cmd, "./corels -r %f -c 2 -p 1 -d -o %s ../data/compas_train.out ../data/compas_train.label ../data/compas_train.minor", r, "falling.csv");
        system(cmd);
        r /= 1.25;
    }
    system("cat default.csv | uniq > default1.csv");
    rename("default1.csv", "default.csv");
    system("cat falling.csv | uniq > falling1.csv");
    rename("falling1.csv", "falling.csv");
}