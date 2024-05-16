#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdatomic.h>

int A;
atomic_int *ulaz;
atomic_int *broj;

int max(atomic_int array[], int n) {
    int max = array[0];
    for(int i = 1; i < n; i++) {
        if(array[i] > max) 
            max = array[i];
    }
    return max;
}

void enterKO(int i, int n) {

    ulaz[i] = 1;
    broj[i] = max(broj, n) + 1;
    ulaz[i] = 0;

    for(int j = 0; j < n; j++) {
        while(ulaz[j] != 0);
        while(broj[j] != 0 && (broj[j] < broj[i] || (broj[j] == broj[i] && j < i)));
    }

    printf("Dretva %d u KO, A = %d. \n", i, A);
}

void exitKO(int i) {
    broj[i] = 0;
    printf("Dretva %d izlazi iz KO, A = %d. \n\n", i, A);

}

void *dretva(void *arg) {

    int broj_dretve = *((int *)arg);
    int n = *((int *)arg + 1);
    int m = *((int *)arg + 2);
    int i = m;
    printf("U dretvi %d. \n \n", broj_dretve);

    do {
        enterKO(broj_dretve, n);
        A = A + 1;
        printf("Dretva %d, A = %d, m = %d. \n", broj_dretve, A, i);
        exitKO(broj_dretve);
        i--;
    } while (i > 0);
}

int main() {

    A = 0;

    int n, m;
    printf("Upišite n: \n");
    scanf("%d", &n);
    printf("Upišite m: \n");
    scanf("%d", &m);

    pthread_t id[n];

    int argumenti[3];
    argumenti[1] = n;
    argumenti[2] = m;

    ulaz = calloc(n, sizeof(int));
    broj = calloc(n, sizeof(int));

    for(int i = 0; i < n; i++) {
        argumenti[0] = i;
        if(pthread_create(&id[i], NULL, dretva, &argumenti) != 0) {
            printf("Greska pri stvaranju dretve %d!\n\n", i);
            exit(1);
        }
        printf("Odrađena dretva %d \n\n", i);
        sleep(1);
    }

    for(int i = 0; i < n; i++) {
        pthread_join(id[i], NULL);
    }

    free(ulaz);
    free(broj);

    return 0;
}