#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <semaphore.h>

int id_brMjesta;
int *brMjesta;

int id_trenutniKlijent;
int *trenutniKlijent;

int id_otvoreno;
int *otvoreno;

int id_krajRadnogVremena; 
int *krajRadnogVremena;

int id_sljedeciKlijent; 
int *sljedeciKlijent;

int id_semOtvoreno;
sem_t *sem_otvoreno;
 
int id_semPostojiKlijent; 
sem_t *sem_postojiKlijent;

int id_semSljedeciKlijent; 
sem_t *sem_sljedeciKlijent;

int id_semNeMoguSpavati; 
sem_t *sem_NeMoguSpavati;

int id_semNemaSljedeceg; 
sem_t *sem_nemaSljedeceg;

int id_semRadim;
sem_t *sem_radim; 

void Brisi(int sig) {
    
    /*oslobađanje zajedničke memorije*/
    (void) shmdt((char *) brMjesta);
    (void) shmctl(id_brMjesta, IPC_RMID, NULL);
    (void) shmdt((char *) otvoreno);
    (void) shmctl(id_otvoreno, IPC_RMID, NULL);
    (void) shmdt((char *) krajRadnogVremena);
    (void) shmctl(id_krajRadnogVremena, IPC_RMID, NULL);
    (void) shmdt((char *) trenutniKlijent);
    (void) shmctl(id_trenutniKlijent, IPC_RMID, NULL);
    (void) shmdt((char *) sljedeciKlijent);
    (void) shmctl(id_sljedeciKlijent, IPC_RMID, NULL);

    (void) shmdt((sem_t *) sem_otvoreno);
    (void) shmctl(id_semOtvoreno, IPC_RMID, NULL);
    (void) shmdt((sem_t *) sem_postojiKlijent);
    (void) shmctl(id_semPostojiKlijent, IPC_RMID, NULL);
    (void) shmdt((sem_t *) sem_sljedeciKlijent);
    (void) shmctl(id_semSljedeciKlijent, IPC_RMID, NULL);
    (void) shmdt((sem_t *) sem_NeMoguSpavati);
    (void) shmctl(id_semNeMoguSpavati, IPC_RMID, NULL);
    (void) shmdt((sem_t *) sem_nemaSljedeceg);
    (void) shmctl(id_semNemaSljedeceg, IPC_RMID, NULL);
    (void) shmdt((sem_t *) sem_radim);
    (void) shmctl(id_semRadim, IPC_RMID, NULL);}

void frizerka() {
    printf("Frizerka: Otvaram salon\n");
    printf("Frizerka: Postavljam znak OTVORENO\n");
    sem_wait(sem_otvoreno);
    *otvoreno = 1;
    sem_post(sem_otvoreno);

    sleep(0.5);

    while(1 == 1) {
        if(*krajRadnogVremena == 1 && *otvoreno == 1) {
            printf("Frizerka: Postavljam znak ZATVORENO\n");
            sem_wait(sem_otvoreno);
            *otvoreno = 0;
            sem_post(sem_otvoreno);
        }

        if(*brMjesta < 5) {
            *trenutniKlijent = *sljedeciKlijent;
            sem_wait(sem_sljedeciKlijent);
            *sljedeciKlijent = 0;
            sem_post(sem_sljedeciKlijent);
            sem_post(sem_nemaSljedeceg);

            printf("Frizerka: Idem raditi na klijentu %d\n", *trenutniKlijent);
            sem_post(sem_radim);

            sleep(1);
            printf("Frizerka: Klijent %d gotov \n", *trenutniKlijent);
            *trenutniKlijent = 0;
        } 
        else if(*krajRadnogVremena != 1) {
            printf("Frizerka: Spavam dok klijenti ne dođu\n");
            sem_wait(sem_NeMoguSpavati);
            sem_post(sem_NeMoguSpavati);
        }
        else {
            if(*otvoreno == 1) {
                printf("Frizerka: Postavljam znak ZATVORENO\n");
                sem_wait(sem_otvoreno);
                *otvoreno = 0;
                sem_post(sem_otvoreno);
            }
            printf("Frizerka: Zatvaram salon\n");
            break;
        } 
    }
}

void klijent(int broj_klijenta) {
    printf("\tKlijent(%d):  Želim na frizuru.\n", broj_klijenta);
    if(*otvoreno == 1 && *brMjesta > 0) {
        sem_wait(sem_postojiKlijent);
        *brMjesta = *brMjesta - 1;
        sem_post(sem_postojiKlijent);
        printf("\tKlijent(%d):  Ulazim u čekaonicu(%d).\n", broj_klijenta, (5 - *brMjesta));
        sem_wait(sem_nemaSljedeceg); // ovog ubaci, inicijalno = 1
        sem_wait(sem_sljedeciKlijent);
        *sljedeciKlijent = broj_klijenta;
        sem_post(sem_sljedeciKlijent);

        sem_post(sem_NeMoguSpavati);

        while(*trenutniKlijent != broj_klijenta);
        sem_wait(sem_radim);

        sem_wait(sem_NeMoguSpavati);

        sem_wait(sem_postojiKlijent);
        *brMjesta = *brMjesta + 1;

        sem_post(sem_postojiKlijent);
        printf("\tKlijent(%d):  frizerka mi radi frizuru\n", broj_klijenta);
        while(*trenutniKlijent != 0);
    } else {
        printf("\tKlijent(%d):  Nema mjesta u čekaoni, vratit ću se sutra\n", broj_klijenta);
    }

}

int main() {

    /*zauzimanje zajedničke memorije*/
    id_brMjesta = shmget(IPC_PRIVATE, sizeof(int), 0600);
    id_otvoreno = shmget(IPC_PRIVATE, sizeof(int), 0600);
    id_krajRadnogVremena = shmget(IPC_PRIVATE, sizeof(int), 0600);
    id_trenutniKlijent = shmget(IPC_PRIVATE, sizeof(int), 0600);
    id_sljedeciKlijent = shmget(IPC_PRIVATE, sizeof(int), 0600);

    id_semOtvoreno = shmget(IPC_PRIVATE, sizeof(sem_t), 0600);
    id_semPostojiKlijent = shmget(IPC_PRIVATE, sizeof(sem_t), 0600);
    id_semSljedeciKlijent = shmget(IPC_PRIVATE, sizeof(sem_t), 0600);
    id_semNeMoguSpavati = shmget(IPC_PRIVATE, sizeof(sem_t), 0600);
    id_semNemaSljedeceg = shmget(IPC_PRIVATE, sizeof(sem_t), 0600);
    id_semRadim = shmget(IPC_PRIVATE, sizeof(sem_t), 0600);

    if(id_brMjesta == -1 || id_otvoreno == -1 || id_krajRadnogVremena == -1 || id_trenutniKlijent == -1 || id_sljedeciKlijent == -1 ||
        id_semOtvoreno == -1 || id_semPostojiKlijent == -1 ||id_semSljedeciKlijent == -1 || id_semNeMoguSpavati == -1 || id_semNemaSljedeceg == -1 || id_semRadim == -1) {
        exit(1);
    }

    brMjesta = (int *) shmat(id_brMjesta, NULL, 0);
    otvoreno = (int *) shmat(id_otvoreno, NULL, 0);
    krajRadnogVremena = (int *) shmat(id_krajRadnogVremena, NULL, 0);
    trenutniKlijent = (int *) shmat(id_trenutniKlijent, NULL, 0);
    sljedeciKlijent = (int *) shmat(id_sljedeciKlijent, NULL, 0);

    sem_otvoreno = shmat(id_semOtvoreno, NULL, 0);
    sem_postojiKlijent = shmat(id_semPostojiKlijent, NULL, 0);
    sem_sljedeciKlijent = shmat(id_semSljedeciKlijent, NULL, 0);
    sem_NeMoguSpavati = shmat(id_semNeMoguSpavati, NULL, 0);
    sem_nemaSljedeceg = shmat(id_semNemaSljedeceg, NULL, 0);
    sem_radim = shmat(id_semRadim, NULL, 0);

    *brMjesta = 5;
    *otvoreno = 0;
    *krajRadnogVremena = 0;
    *trenutniKlijent = 0;
    *sljedeciKlijent = 0;
    sem_init(sem_otvoreno, 1, 1);
    sem_init(sem_sljedeciKlijent, 1, 1);
    sem_init(sem_postojiKlijent, 1, 1);
    sem_init(sem_NeMoguSpavati, 1, 0);
    sem_init(sem_nemaSljedeceg, 1, 1);
    sem_init(sem_radim, 1, 0);

    if(fork() == 0) {
        frizerka();
        exit(0);
    }
    printf("Krecemo stvarati klijente\n");
    int i;

    for(i = 0; i < 4; i++) {
        if(fork() == 0) {
            klijent(i+1);
            exit(0);
        }
    }

    for(; i < 6; i++) {
        sleep(3);
        if(fork() == 0) {
            klijent(i+1);
            exit(0);
        }
    }

    *krajRadnogVremena = 1;

    for(i = i + 1; i >= 0; i--) {
        (void) wait(NULL);  // klijenti i frizerka
    }

    sem_destroy (sem_otvoreno);
    sem_destroy (sem_sljedeciKlijent);
    sem_destroy (sem_NeMoguSpavati);
    sem_destroy (sem_postojiKlijent);
    sem_destroy (sem_nemaSljedeceg);
    sem_destroy (sem_radim);

    Brisi(0); 
}
