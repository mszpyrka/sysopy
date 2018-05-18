#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv) {

    int size = atoi(argv[1]);

    srand(time(NULL));

    double *tab = malloc(sizeof(double) * size * size);
    double sum = 0.0;

    for(int i = 0; i < size * size; i++) {
        tab[i] = (double) (rand() % 50);
        sum += tab[i];
    }

    for(int i = 0; i < size * size; i++)
        tab[i] /= sum;

    printf("%d\n", size);

    for(int i = 0; i < size; i++) {
        for(int j = 0; j < size; j++)
            printf("%lf ", tab[size * i + j]);

        printf("\n");
    }
}
