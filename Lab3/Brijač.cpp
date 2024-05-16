#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>

using namespace std;

int brMjesta;
int brZauzetihMjesta;

int otvoreno;
int krajRadnogVremena;
int postojiKlijent;
int obradujeKlijenta;

pthread_mutex_t m; //monitor
pthread_cond_t cond[2]; // [0] brijac ceka da ima klijenta, [1] klijent ceka da ga brijac primi na brijanje

void *brijac(void *x) {
    
    pthread_mutex_lock(&m);
    otvoreno = 1;
    cout << "Brijac: Otvaram brijacnicu\n";
    pthread_mutex_unlock(&m);

    while(true) {
        if(krajRadnogVremena == 1 && otvoreno == 1) {
            pthread_mutex_lock(&m);
            otvoreno = 0;
            cout << "Brijac: Postavljam znak ZATVORENO\n";
            pthread_mutex_unlock(&m);
        }
        if(brZauzetihMjesta > 0) {
            pthread_mutex_lock(&m);
            obradujeKlijenta = 1;
            pthread_cond_signal(&cond[1]);
            pthread_mutex_unlock(&m);
            sleep(2);
            cout << "Brijac: Klijent gotov\n";
        } else {
            if(!krajRadnogVremena) {
                pthread_mutex_lock(&m);
                cout << "Brijac: Spim dok klijenti ne dodu\n";
                while(postojiKlijent == 0) {
                    pthread_cond_wait(&cond[0], &m);
                }
                cout << "Brijac: Idem raditi\n";
                pthread_mutex_unlock(&m);
            } else {
                cout << "Brijac: Zatvaram brijačnicu\n";
                break;
            }
        }
    }

}

void *klijent(void *id) {
    int *id_klijent = (int*)id;
    cout << "       Klijent: (" << *id_klijent << "): Zelim na brijanje\n";
    if(otvoreno && brZauzetihMjesta < brMjesta) {
        pthread_mutex_lock(&m);
        brZauzetihMjesta++;
        cout << "       Klijent: (" << *id_klijent << "): Ulazim u čekaonicu (" << brZauzetihMjesta << ")\n";
        
        postojiKlijent = 1;
        pthread_cond_signal(&cond[0]);
        
        while(obradujeKlijenta == 0) {
            pthread_cond_wait(&cond[1], &m);
        }
        obradujeKlijenta = 0;
        brZauzetihMjesta--;
        postojiKlijent = 0;
        pthread_mutex_unlock(&m);

        cout << "       Klijent: (" << *id_klijent << "): Brijac me brije\n";
        //jel treba sinkronizirat da čeka da je gotov ili ne?
    } else {
        cout << "       Klijent: (" << *id_klijent << "): Nema mjesta u cekaonici\n";
    }

}

void *stvaranje(void *x) {

    pthread_t thr_id[10];


    if(pthread_create(&thr_id[0], NULL, brijac, NULL) == -1) {
        cout << "Greška pri stvaranju dretve brijac.\n";
    }

    sleep(1);
    
    int id_klijent[10];
    for(int i = 1; i < 10; i++) {
        id_klijent[i] = i;
        if(pthread_create(&thr_id[i], NULL, klijent, &id_klijent[i]) == -1) {
            cout << "Greška pri stvaranju dretve klijenta.\n";
        } 
        sleep(1.5);
    }

    for(int i = 0; i < 10; i++) {
        pthread_join(thr_id[i], NULL);
    }
}

int main(void) {

    pthread_t thr_id[1];

    pthread_mutex_init (&m, NULL); //inicijalizacija
    pthread_cond_init (&cond[0], NULL);
    pthread_cond_init (&cond[1], NULL);

    cout << "Upišite broj mjesta u čekaonici: \n";
    cin >> brMjesta;

    brZauzetihMjesta = 0;
    otvoreno = 0;
    krajRadnogVremena = 0;
    postojiKlijent = 0;

    if(pthread_create(&thr_id[0], NULL, stvaranje, NULL) == -1) {
        cout << "Greška pri stvaranju dretve Stvaranje.\n";
    }
    
    sleep(15);

    pthread_mutex_lock(&m);
    krajRadnogVremena = 1;
    cout << "Kraj radnog vremena!\n";
    pthread_cond_signal(&cond[0]);
    pthread_mutex_unlock(&m);

    pthread_join(thr_id[0], NULL);
    

    pthread_mutex_destroy (&m);
    pthread_cond_destroy (&cond[0]);
    pthread_cond_destroy (&cond[1]);

    return 0;
}
