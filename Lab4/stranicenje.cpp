#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <iomanip>
#include <limits>

using namespace std;

int t = 0; //LRU
int n; //broj procesa
int m; //broj okvira

int zauzeti; // koliko smo u okviru mjesta zauzeli

vector<vector<int>> tablica;
vector<vector<int>> okvir;
vector<vector<int>> disk;

string formatHex(int number) { // pomoć za ispis heksadecimalnih brojeva
    ostringstream oss;
    oss << "0x" << setfill('0') << setw(4) << hex << number;
    return oss.str();
}

void zapisi_vrijednost(int p, int stupac_okvira) { // funkcija za zapisivanje podataka na disk
    for(auto i = 0; i < 64; i++) { // pohranjujemo na disk sa okvira
        disk[p][i] = okvir[stupac_okvira][i]; 
    }
}

int dohvati_fizicku_adresu(int p, int virtualna_adr) {
    int indeks = virtualna_adr >> 6; // indeks tablice straničenja, 7, 8, 9 i 10 bit
    int pomak = virtualna_adr & 0x003f; // pomak unutar okvira, prvih 6 bita 
    int zapis = tablica[p][indeks]; // zapis u tablici prevođenja za zadani proces i indeks tablice straničenja
    int bit_prisutnosti = (zapis >> 5) & 1;

    if(bit_prisutnosti == 0) {
        cout << "\tPromašaj! \n";
        if(zauzeti < m) {
            cout << "\t\tdodijeljen okvir: " << formatHex(zauzeti) << ".\n";
            zapis = zauzeti << 6;  //fizicka adresa okvira
            zapis = zapis + 32 + t; //bit prisutnosti + LRU
            for(auto i = 0; i < 64; i++) {
                okvir[zauzeti][i] = disk[p][i]; //ucitavamo sadržaj stranice s diska 
            }
            zauzeti++; // povecavamo broj zauzetih stranica u okviru
            tablica[p][indeks] = zapis;
        }
        else {
            //trebamo pronaći i maknuti stranicu sa najmanjim LRU
            int min_lru = 31; // najveci moguci LRU + 1
            int min_okvir;
            int min_zapis;
            int min_proces;
            int min_stranica;
            for(auto i = 0; i < n; i++) { // prolazimo kroz cijelu tablicu i gledamo od prisutnih koji ima najmanji LRU
                for(auto j = 0; j < 16; j++) {
                    if((tablica[i][j] & 0x0020) == 0x0020 && (tablica[i][j] & 0x001f) < min_lru) {
                        // bit prisutnosti = 1 te ima LRU manji od trenutno minimalnog
                        min_zapis = tablica[i][j];
                        min_proces = i;
                        min_stranica = j;
                    }
                }
            }
            // pronašli smo zapis sa min LRU, izdvajamo lru i okvir, ispis te trebamo zapisat vrijednosti na disk
            min_lru = zapis & 0x001f;
            min_okvir = (zapis & 0xffc0) >> 6;
            cout << "\t\tIzbacujem stranicu " << formatHex(min_stranica * 64) << " iz procesa " << min_proces << "\n";
            cout << "\t\tlru izbacene stranice: " << formatHex(min_lru) << "\n";
            cout << "\t\tdodijeljen okvir " << formatHex(min_okvir) << "\n";
            zapisi_vrijednost(min_proces, min_okvir);

            for(auto i = 0; i < n; i++) {
                for(auto j = 0; j < 16; j++) {
                    if(tablica[i][j] == min_zapis) {
                        tablica[i][j] = tablica[i][j] & 0xffdf; // postavljamo bit prisutnosti zapisa sa min LRU na 0
                    }
                }
            }

            for(auto i = 0; i < 64; i++) { // stranicu sa diska ucitavamo u okvir
                okvir[min_okvir][i] = disk[p][i]; 
            }

            tablica[p][indeks] = (min_okvir << 6); //fizička adresa okvira
            tablica[p][indeks] = tablica[p][indeks] + 32 + t; //bit prisutnosti + LRU metapodatak
        }
    }
    int fiz_adresa = (zapis & 0xffc0) + pomak;

    if(t == 31) {
        for(auto i = 0; i < n; i++) {
            for(auto j = 0; j < 16; j++) { // postavljamo sve LRU na 0
                tablica[i][j] = tablica[i][j] & 0xffe0;
            }
        }
        tablica[p][indeks] = tablica[p][indeks] & 0xffe1; // postavljamo LRU trenutne stranice na 1
        t = 1;
    }

    return fiz_adresa;

}

int dohvati_sadrzaj(int p, int virtualna_adr) {

    cout << "\tt: " << t << "\n";
    cout << "\tlog. adresa: " <<  formatHex(virtualna_adr) << "\n";

    int fiz_adresa = dohvati_fizicku_adresu(p, virtualna_adr); 
    int zapis = tablica[p][virtualna_adr >> 6];

    cout << "\tfiz. adresa: " << formatHex(fiz_adresa) << "\n";
    cout << "\tzapis tablice: " << formatHex(zapis) << "\n";

    // for(vector<int> red : okvir) {
    //     for(int el : red) {
    //         cout << el << " ";
    //     }
    //     cout << "\n";
    // }
    // cout << (fiz_adresa & 0xffc0) << " " << (fiz_adresa & 0x003f) << "\n";
    
    int vrijednost = okvir[fiz_adresa >> 6][fiz_adresa & 0x003f];

    cout << "\tsadrzaj adrese: " << vrijednost << "\n";

    return vrijednost;
}

void zapisi_sadrzaj(int p, int virtualna_adr, int vrijednost) {
    int fiz_adresa = dohvati_fizicku_adresu(p, virtualna_adr);
    okvir[fiz_adresa >> 6][fiz_adresa & 0x003f] = vrijednost;
}

int main() {

    cout << "upisite n: \n"; cin >> n;
    cout << "upisite m: \n"; cin >> m;

    for (int i = 0; i < n; ++i) {
        tablica.push_back(vector<int>(16));
        for (int j = 0; j < 16; ++j) {
            tablica[i][j] = 0;
        }
    }

    for (int i = 0; i < m; ++i) {
        okvir.push_back(vector<int>(64));
        for (int j = 0; j < 64; ++j) {
            okvir[i][j] = 0;
        }
    }
    
    for(auto i = 0; i < n; i++) {
        disk.push_back(vector<int>(64));
        for (auto j = 0; j < 64; j++) {
            disk[i][j] = 0;
        }
    }

    srand((unsigned)time(0));
    int virtualna_adresa;
    zauzeti = 0;

    while(true) {
        for(auto p = 0; p < n; p++) {
            virtualna_adresa = rand();
            virtualna_adresa = virtualna_adresa & 0x03fe;
            cout << "--------------------------------------------\n";
            cout << "proces: " << p << "\n";
            int a = dohvati_sadrzaj(p, virtualna_adresa);
            a++;
            zapisi_sadrzaj(p, virtualna_adresa, a);
            t++;
            this_thread::sleep_for(chrono::seconds(1));
        }
    }

    return 0;
}