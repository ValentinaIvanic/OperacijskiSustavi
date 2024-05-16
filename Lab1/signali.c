#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>


struct timespec t0;     // početno vrijeme
int zastavice[3] = {0};
int prioritet = 0;
int stog[3];
int top = -1;
int signal_U_Obradi[3] = {-1, -1, -1};


// postavljamo početno vrijeme
void SetPocetnoVrijeme() {
    clock_gettime(CLOCK_REALTIME, &t0);
}

// računa i ispisuje vrijeme rada programa
void ProtekloVrijeme() {

    struct timespec t;      // trenutno vrijeme
    clock_gettime(CLOCK_REALTIME, &t);

    t.tv_sec -= t0.tv_sec;
    t.tv_nsec -= t0.tv_nsec;
    if (t.tv_nsec < 0) {
        t.tv_nsec += 1000000000;
        t.tv_sec--;
    }
    printf("%03ld.%03ld:\t", t.tv_sec, t.tv_nsec/1000000);
}

// ispis za automatski ispis vremena uz svaki printf
#define PRINTF(format, ...)             \
do {                                    \
    ProtekloVrijeme();                          \
    printf(format, ##__VA_ARGS__);      \
} while(0);

// ispis za stanje sustava
void printStanjeSustava() {
    PRINTF("K_Z = %d%d%d, T_P = %d, stog: ", zastavice[0], zastavice[1], zastavice[2], prioritet);
    if(top == -1) {
        printf("- \n\n");
    }
    else {
        for(int i = top; i >= 0; i--) {
            printf("%d reg[%d]; ", stog[i], stog[i]);
        }
        printf("\n\n");
    }
}


// obrada prekida
void pozvan_signal(int razina) {

    top++;
    stog[top] = prioritet;
    prioritet = razina;

    zastavice[razina - 1] = 0;
    PRINTF("Počela obrada razine %d. \n", razina);
    printStanjeSustava();

    for(int j = 0; j < 3; j++) {
        signal_U_Obradi[j] = -1;
    }
    signal_U_Obradi[razina - 1] = 1;

    time_t sekunde = 15;
    struct timespec duljina_spavanja;
    duljina_spavanja.tv_sec = sekunde;
    duljina_spavanja.tv_nsec = 0;

    while(nanosleep(&duljina_spavanja, &duljina_spavanja) == -1 && errno == EINTR) {
        if(signal_U_Obradi[razina - 1] == -1) {
            signal_U_Obradi[razina - 1] = 1;
            PRINTF("Nastavlja se obrada prekida razine %d. \n", razina);
            printStanjeSustava();
        }
    }

    PRINTF("Završila obrada prekida razine %d. \n\n", razina);
    signal_U_Obradi[razina - 1] = -1;

    prioritet = stog[top];
    stog[top] = -1;
    top--;
}

void spavaj(time_t sekunde) {

    struct timespec duljina_spavanja;
    duljina_spavanja.tv_sec = sekunde;
    duljina_spavanja.tv_nsec = 0;

    while(nanosleep(&duljina_spavanja, &duljina_spavanja) == -1 && errno == EINTR) {
        PRINTF("Nastavlja se izvođenje glavnog programa. \n");
        printStanjeSustava();

        bool signal_Ceka = false;
        for(int i = 2; i >= 0; i--) {
            if(zastavice[i] == 1) {
                signal_Ceka = true;
                PRINTF("SKLOP: promijenio se T_P, prosljeđuje prekid razine %d procesoru. \n", i + 1);
                pozvan_signal(i + 1);
                break;
            }
        }

        if(signal_Ceka) {
            PRINTF("Nastavlja se izvođenje glavnog programa. \n");
            printStanjeSustava();
        }
    }

}

void obradi_sigint(int sig) {       // prekid najviše razine(3)
    zastavice[2] = 1;
    PRINTF("Dogodio se prekid razine 3")
    if(prioritet < 3) {
        printf(" te se prosljeđuje procesoru. \n");
        printStanjeSustava();
        pozvan_signal(3);
    }
    else {
        printf(", pamti se i ne prosljeđuje procesoru.\n");
        printStanjeSustava();
    }
}

void obradi_sigusr1(int sig) {      // prekid srednje razine(2)
    zastavice[1] = 1;
    PRINTF("Dogodio se prekid razine 2")
    if(prioritet < 2) {
        printf(" te se prosljeđuje procesoru. \n");
        printStanjeSustava();
        pozvan_signal(2);
    }
    else {
        printf(", pamti se i ne prosljeđuje procesoru.\n");
        printStanjeSustava();
    }
}

void obradi_sigterm(int sig) {      // prekid najniže razine(1)
    zastavice[0] = 1;
    PRINTF("Dogodio se prekid razine 1")
    if(prioritet < 1) {
        printf(" te se prosljeđuje procesoru. \n");
        printStanjeSustava();
        pozvan_signal(1);
    }
    else {
        printf(", pamti se i ne prosljeđuje procesoru.\n");
        printStanjeSustava();
    }
}


void inicijalizacija() {

    struct sigaction act;

    act.sa_handler = obradi_sigint;    //funckija za obradu signala
    sigemptyset(&act.sa_mask);          
    act.sa_flags = 0;                   // ne koristimo naprednije mogućnosti
    sigaction(SIGINT, &act, NULL);

	act.sa_handler = obradi_sigusr1;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGUSR1, &act, NULL);

    act.sa_handler = obradi_sigterm;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	sigaction(SIGTERM, &act, NULL);

    SetPocetnoVrijeme();

}

int main() {

    inicijalizacija();

    PRINTF("Program s PID = %d krenuo s radom. \n", getpid());
    PRINTF("K_Z = %d%d%d, T_P = %d, stog: - \n\n", zastavice[0], zastavice[1], zastavice[2], prioritet);

    for(;;)
        spavaj(1000);

    return 0;
}
