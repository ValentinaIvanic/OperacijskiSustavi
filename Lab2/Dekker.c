#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>

int id_Pravo;
int *Pravo;
int id_Z; // identifikacijski broj segmenta
int *Zastavice;
int id_A;
int *A;

void Brisi(int sig) {
    
    /*oslobađanje zajedničke memorije*/
    (void) shmdt((char *) Pravo);
    (void) shmctl(id_Pravo, IPC_RMID, NULL);
    (void) shmdt((char *) Zastavice);
    (void) shmctl(id_Z, IPC_RMID, NULL);
    (void) shmdt((char *) A);
    (void) shmctl(id_A, IPC_RMID, NULL);
}

void enter_KO(int i, int j) {
    *(Zastavice + i) = 1;
    while (*(Zastavice + j) != 0) {
        if (*Pravo == j) {
            *(Zastavice + i) = 0;
            while (*Pravo == j);
            *(Zastavice + i) = 1;
        }
    }
    printf("Proces %d u KO, A = %d. \nPravo = %d, Zastavice[0] = %d, Zastavice[1] = %d \n", i, *A, *Pravo, *Zastavice, *(Zastavice + 1));
}

void exit_KO(int i, int j) {
    *Pravo = j;
    *(Zastavice + i) = 0;
    printf("Proces %d izlazi iz KO, A = %d. \nPravo = %d, Zastavice[0] = %d, Zastavice[1] = %d \n \n", i, *A, *Pravo, *Zastavice, *(Zastavice + 1));
}


void proces(int i, int m) {
    printf("Novi proces %d. \n", i);
    do {
        enter_KO(i, (i + 1) % 2);
        *A = *A + 1;
        printf("Proces %d, A = %d. \n", i, *A);
        exit_KO(i, (i + 1) % 2);
        sleep(0.5);
        m--;
    } while (m > 0);
}

int main() {
    /*zauzimanje zajedničke memorije*/
    id_Pravo = shmget(IPC_PRIVATE, sizeof(int), 0600);
    id_Z = shmget(IPC_PRIVATE, sizeof(int), 0600);
    id_A = shmget(IPC_PRIVATE, sizeof(int), 0600);

    if(id_Pravo == -1 || id_Z == -1 || id_A == -1) {
        exit(1);
    }

    Pravo = (int *) shmat(id_Pravo, NULL, 0);
    *Pravo = 0;

    Zastavice = (int *) shmat(id_Z, NULL, 0);
    *Zastavice  = 0; // ZASTAVICA[0]
    *(Zastavice + 1) = 0; // ZASTAVICA[1]

    A = (int *) shmat(id_A, NULL, 0);
    *A = 0;

    int m;
    printf("Upišite m: \n");
    scanf("%d", &m);

    printf("A = %d, m = %d \n \n", *A, m);

    if(fork() == 0) {
        proces(0, m);
        exit(0);
    }

    if(fork() == 0) {
        proces(1, m);
        exit(0);
    }

    (void) wait(NULL);
    (void) wait(NULL);
    Brisi(0);

    return 0;
}