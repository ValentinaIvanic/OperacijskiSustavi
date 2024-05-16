#include <pthread.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int obala_camca;  // gdje se camac nalazi, lijeva obala = 0, desna obala = 1
char *obala_camcaPrvoSlovo[] = {"L", "D"}; // za funkciju ispis
char *obala_osobe[] = {"lijevu", "desnu"}; // pomoc za ispis na kojoj strani se nalazi misionar/kanibal
char *obala_camacCeka[] = {"lijevoj", "desnoj"}; //prazan na x obali
char *obala_camacPocetna[] = {"lijeve", "desne"}; // s koje obale prevozi
char *obala_camacZavrsna[] = {"lijevu", "desnu"}; // na koju obalu prevozi

int ukrcani;
int ukrcani_misionari;
int ukrcani_kanibali;

int voznja_traje; // 1 ako je voznja u tijeku

int broj_osobaNaObali[2]; // lijeva = 0, desna = 1, broj kanibala+misionara na pojedinoj obali
int size_Obale[2];   // lijeva = 0, desna = 1, za realloc

int *lijevaObala_vrsta; // misionar = 0, kanibal = 1
int *lijevaObala_broj; // redni broj misionara/kanibala

int *desnaObala_vrsta; // misionar = 0, kanibal = 1
int *desnaObala_broj; // redni broj misionara/kanibala

// za camac imamo varijablu ukrcani
int *camac_vrsta;  // misionar = 0, kanibal = 1
int *camac_broj;

pthread_mutex_t m;
pthread_cond_t red[2]; // lijeva i desna obala

void ispis() {
    printf("C[%s] = {", obala_camcaPrvoSlovo[obala_camca]);

    for(int i = 0; i < ukrcani; i++) {
        if(camac_vrsta[i] == 0)
            printf("M");
        else 
            printf("K");
        
        printf("%d ", camac_broj[i]);
    }
    printf("} LO = {");

    for(int i = 0; i < broj_osobaNaObali[0]; i++) {
        if(lijevaObala_vrsta[i] == 0)
            printf("M");
        else if(lijevaObala_vrsta[i] == 1)
            printf("K");
        
        if(lijevaObala_broj[i] != -1) {
            printf("%d ", lijevaObala_broj[i]);
        }

    }
    printf("} DO = {");

    for(int i = 0; i < broj_osobaNaObali[1]; i++) {
        if(desnaObala_vrsta[i] == 0)
            printf("M");
        else if(desnaObala_vrsta[i] == 1)
            printf("K");
        
        if(desnaObala_broj[i] != -1) {
            printf("%d ", desnaObala_broj[i]);        
        }
    }
    printf("}\n\n");
}

void ispis_camac() {
    for(int i = 0; i < ukrcani; i++) {
        if(camac_vrsta[i] == 0)
            printf("M");
        else 
            printf("K");
        
        if(i < ukrcani - 1)
            printf("%d ", camac_broj[i]);
        else
            printf("%d", camac_broj[i]);
    }
    printf("\n\n");
}

void *camac(void *arg) {
    while (1 == 1) {
        pthread_mutex_lock(&m);
        printf("C: prazan na %s obali\n", obala_camacCeka[obala_camca]);
        ispis();
        pthread_mutex_unlock(&m);

        pthread_cond_signal(&red[obala_camca]);
        pthread_cond_signal(&red[obala_camca]);
        pthread_cond_signal(&red[obala_camca]);

        while(ukrcani < 3);
        pthread_mutex_lock(&m);
        printf("C: tri putnika ukrcana, polazim za jednu sekundu\n");
        ispis();
        pthread_mutex_unlock(&m);
        
        pthread_cond_broadcast(&red[obala_camca]);

        sleep(1);
        voznja_traje = 1;
        pthread_mutex_lock(&m);
        printf("C: prevozim s %s na %s obalu: ", obala_camacPocetna[obala_camca], obala_camacZavrsna[1 - obala_camca]);
        ispis_camac();
        pthread_mutex_unlock(&m);

        sleep(2);
        printf("C: preveo s %s na %s obalu: ", obala_camacPocetna[obala_camca], obala_camacZavrsna[1 - obala_camca]);
        ispis_camac();

        ukrcani = 0;
        voznja_traje = 0;
        obala_camca = 1 - obala_camca;
    }
}

void *kanibal(void *arg) {
    int broj_kanibala = *((int *)arg);
    int obala_kanibala = *((int *)arg + 1);
    int pozicija_na_obali;

    pthread_mutex_lock (&m);

    if(obala_kanibala == 0) {
        lijevaObala_vrsta[broj_osobaNaObali[0]] = 1;
        lijevaObala_broj[broj_osobaNaObali[0]] = broj_kanibala;
    } else {
        desnaObala_vrsta[broj_osobaNaObali[1]] = 1;
        desnaObala_broj[broj_osobaNaObali[1]] = broj_kanibala;
    }
    pozicija_na_obali = broj_osobaNaObali[obala_kanibala];

    if((broj_osobaNaObali[obala_kanibala]++) == size_Obale[obala_kanibala] - 1) {
        size_Obale[obala_kanibala] = size_Obale[obala_kanibala] + 10;
        if(obala_kanibala == 0) {
            lijevaObala_broj = (int*)realloc(lijevaObala_broj, size_Obale[obala_kanibala] * sizeof(int));
            lijevaObala_broj = (int*)realloc(lijevaObala_vrsta, size_Obale[obala_kanibala] * sizeof(int));
        }
        else {
            desnaObala_broj = (int*)realloc(desnaObala_broj, size_Obale[obala_kanibala] * sizeof(int));
            desnaObala_vrsta = (int*)realloc(desnaObala_vrsta, size_Obale[obala_kanibala] * sizeof(int));
        }
    }

    printf("K%d: došao na %s obalu\n", broj_kanibala, obala_osobe[obala_kanibala]);
    ispis();
    while(ukrcani >= 7 || voznja_traje || obala_camca != obala_kanibala || (ukrcani > 0 && (ukrcani_misionari == ukrcani_kanibali))) {
        pthread_cond_wait(&red[obala_kanibala], &m);
    }

    if(obala_kanibala == 0) {
        lijevaObala_vrsta[pozicija_na_obali] = -1;
        lijevaObala_broj[pozicija_na_obali] = -1;
    } else {
        desnaObala_vrsta[pozicija_na_obali] = -1;
        desnaObala_broj[pozicija_na_obali] = -1;
    }

    camac_vrsta[ukrcani] = 1;
    camac_broj[ukrcani] = broj_kanibala;
    ukrcani_kanibali++;
    ukrcani++;

    printf("K%d: ušao u čamac\n", broj_kanibala);
    ispis();
    pthread_mutex_unlock(&m);
    
    while(!voznja_traje);
    pthread_mutex_lock(&m);
    ukrcani_kanibali--;
    pthread_mutex_unlock(&m);
    while(voznja_traje);
}

void *misionar(void *arg) {
    int broj_misionara = *((int *)arg);
    int obala_misionara = *((int *)arg + 1);
    int pozicija_na_obali;
    
    pthread_mutex_lock (&m);

    if(obala_misionara == 0) { // ako smo na lijevoj obali 
        lijevaObala_vrsta[broj_osobaNaObali[0]] = 0;
        lijevaObala_broj[broj_osobaNaObali[0]] = broj_misionara;
    } else {
        desnaObala_vrsta[broj_osobaNaObali[1]] = 0;
        desnaObala_broj[broj_osobaNaObali[1]] = broj_misionara;
    }
    pozicija_na_obali = broj_osobaNaObali[obala_misionara];

    if((broj_osobaNaObali[obala_misionara]++) == size_Obale[obala_misionara] - 1) {
        size_Obale[obala_misionara] = size_Obale[obala_misionara] + 10;
        if(obala_misionara == 0) {
            lijevaObala_broj = (int*)realloc(lijevaObala_broj, size_Obale[obala_misionara] * sizeof(int));
            lijevaObala_broj = (int*)realloc(lijevaObala_vrsta, size_Obale[obala_misionara] * sizeof(int));
        }
        else {
            desnaObala_broj = (int*)realloc(desnaObala_broj, size_Obale[obala_misionara] * sizeof(int));
            desnaObala_vrsta = (int*)realloc(desnaObala_vrsta, size_Obale[obala_misionara] * sizeof(int));
        }
    }

    printf("M%d: došao na %s obalu\n", broj_misionara, obala_osobe[obala_misionara]);
    ispis();
    while(ukrcani >= 7 || voznja_traje || obala_camca != obala_misionara || (ukrcani > 0 && ukrcani_misionari == 0 && ukrcani_kanibali > 1)) {
        pthread_cond_wait(&red[obala_misionara], &m);
    }

    if(obala_misionara == 0) {
        lijevaObala_vrsta[pozicija_na_obali] = -1;
        lijevaObala_broj[pozicija_na_obali] = -1;
    } else {
        desnaObala_vrsta[pozicija_na_obali] = -1;
        desnaObala_broj[pozicija_na_obali] = -1;
    }

    camac_vrsta[ukrcani] = 0;
    camac_broj[ukrcani] = broj_misionara;
    ukrcani_misionari++;
    ukrcani++;
    printf("M%d: ušao u čamac\n", broj_misionara);
    ispis();
    pthread_mutex_unlock(&m);

    while(!voznja_traje);
    pthread_mutex_lock(&m);
    ukrcani_misionari--;
    pthread_mutex_unlock(&m);   
    while(voznja_traje);
}

void *stvori_kanibale() {
    pthread_t thr_id[5];
    int arg[2]; // arg = {redni broj kanibala, strana rijeke}

    for(int i = 0; i < 100; i++) {
        int obala_spawn = rand() & 1;
        arg[0] = i;
        arg[1] = obala_spawn;
        if(pthread_create(&thr_id[i], NULL, kanibal, &arg) == -1) {
            printf("Greska pri stvaranju dretve kanibal.\n");
            exit(1);
        }
        sleep(2);
    }
    for(int i = 0; i < 100; i++) {
        pthread_join(thr_id[i], NULL);
    }
}

void *stvori_misionare() {
    pthread_t thr_id[5];
    int arg[2]; // arg = {redni broj misionara, strana rijeke}

    for(int i = 0; i < 100; i++) {
        int obala_spawn = rand() & 1;
        arg[0] = i;
        arg[1] = obala_spawn;

        if(pthread_create(&thr_id[i], NULL, misionar, &arg) == -1) {
            printf("Greška pri stvaranju dretve misionar.\n");
            exit(1);
        }
        sleep(1);
    }

    for(int i = 0; i < 100; i++) {
        pthread_join(thr_id[i], NULL);
    }
    
}

int main() {

    pthread_t thr_id[3];

    pthread_mutex_init (&m, NULL);
    pthread_cond_init (&red[0], NULL);
    pthread_cond_init (&red[1], NULL);

    obala_camca = 1; // desna obala
    ukrcani = 0;
    ukrcani_misionari = 0;
    ukrcani_kanibali = 0;
    voznja_traje = 0;

    broj_osobaNaObali[0] = 0;
    broj_osobaNaObali[1] = 0;
    size_Obale[0] = 50;
    size_Obale[1] = 50;

    lijevaObala_vrsta = (int*)malloc(50 * sizeof(int));
    lijevaObala_broj = (int*)malloc(50 * sizeof(int));
    desnaObala_vrsta = (int*)malloc(50 * sizeof(int));
    desnaObala_broj = (int*)malloc(50 * sizeof(int));
    camac_vrsta = (int*)malloc(8 * sizeof(int));
    camac_broj = (int*)malloc(8 * sizeof(int));

    srand((unsigned int) time(NULL));
    
    if(pthread_create(&thr_id[0], NULL, camac, NULL) == -1) {
        printf("Greška pri stvaranju dretve čamac.\n");
    }
    if(pthread_create(&thr_id[1], NULL, stvori_misionare, NULL) == -1) {
        printf("Greška pri stvaranju dretve stvori_misionare.\n");
    }
    if(pthread_create(&thr_id[2], NULL, stvori_kanibale, NULL) == -1) {
        printf("Greška pri stvaranju dretve stvori_kanibale.\n");
    }
    
    for(int i = 0; i < 3; i++) {
        pthread_join(thr_id[i], NULL);
    }

    pthread_mutex_destroy (&m);
    pthread_cond_destroy (&red[0]);
    pthread_cond_destroy (&red[1]);

    return 0;
}