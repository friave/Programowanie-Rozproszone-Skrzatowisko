#include <pthread.h>
#include <vector>
#include <thread>
#include <chrono>
#include <ctime>
#include <iostream>
#include <cstdlib>
#include <algorithm>
#include <mpi.h>
#include <unistd.h>
#include <stdio.h>

#define W 15
#define KONIE 4 
#define S 6
#define MAX_CZAS 10

#define ZASPOKOJONY 1000
#define CZEKA_NA_KONIA 1001
#define CZEKA_NA_WSTAZKI 1002
#define ZAPLATA 1003

#define NA_PRZERWIE 2000
#define CZEKA_NA_PACJENTA 2001
#define CZEKA_NA_SALKE 2002
#define PRACUJE 2003

#define REQ_KONIE 3000
#define REQ_WSTAZKI 3001
#define REQ_PACJENCI 3002
#define REQ_SALKI 3003

#define ACK_ZGODA 4000

#define INFO_KONIE 5000
#define INFO_WSTAZKI 5001
#define INFO_PACJENCI 5002
#define INFO_SALKI 5003
#define INFO_ZASOBY 5004


pthread_mutex_t lamport_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t kolejka_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t kolejka_ze_wstazkami_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t konie_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wstazki_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pacjenci_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t salki_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t zgody_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wysylanie_mutex = PTHREAD_MUTEX_INITIALIZER;



struct kolejka_info {
  	int id;
  	int lamport;     
    //bool operator==(const struct kolejka_info_s &x) { return true;}

   // bool operator==(const kolejka_info &a) const{
    //    return id==a.id;
    //}
};

struct kolejka_info_wstazki{
  	int id;
  	int lamport;
  	int wstazki;
    //bool operator==(const struct kolejka_info_wstazki_s &x) { return true;}      

    //bool operator==(const kolejka_info_wstazki &a) const{
     //   return id==a.id;
    //}
};

bool operator==(kolejka_info const& a, kolejka_info const& b){
	return a.id == b.id;
}

bool operator==(kolejka_info_wstazki const& a, kolejka_info_wstazki const& b){
	return a.id == b.id;
}


int zegar;

int size,rank;

int konie;
int wstazki;
int salki;

int zgody;

int stan;

kolejka_info moje_zamowienie_koni;
kolejka_info_wstazki moje_zamowienie_wstazki;



std::vector<kolejka_info> kolejka = {};  //dla skrzatow to kolejka o konie, a dla psycholozek o salki
std::vector<kolejka_info_wstazki> kolejka_ze_wstazkami = {}; // dla skrzatow to kolejka o wstazki, a dla psycholozek o pacjentow

void zwieksz_lamporta(int z){
    pthread_mutex_lock(&lamport_mutex);
    zegar=std::max(z,zegar)+1;
    pthread_mutex_unlock(&lamport_mutex);
}

bool por(kolejka_info A,  kolejka_info B){
		if (A.lamport < B.lamport)
			return true;
		if (A.lamport > B.lamport)
			return false;
		else
			return A.id < B.id;
}

bool por_ze_wstazkami(kolejka_info_wstazki A,  kolejka_info_wstazki B){
		if (A.lamport < B.lamport)
			return true;
		if (A.lamport > B.lamport)
			return false;
		else
			return A.id < B.id;

}

int suma_wstazek(std::vector<kolejka_info_wstazki> kolejka_z_info, kolejka_info_wstazki zamowienie) {

    int suma = zamowienie.wstazki;

    for(std::vector<kolejka_info_wstazki>::const_iterator   it = kolejka_z_info.begin(); it != find(kolejka_z_info.begin(), kolejka_z_info.end(), zamowienie); it++ )
    {
        kolejka_info_wstazki element = *it;
        suma += element.wstazki;
    }

    return suma;
}


int ile_jest_przedemna(std::vector<kolejka_info> kolejka_z_info, kolejka_info zamowienie)
{
    auto it = find(kolejka_z_info.begin(), kolejka_z_info.end(), zamowienie);

    if (it != kolejka_z_info.end()) 
    {
        int index = it - kolejka_z_info.begin();
        return index + 1;
    }
    return -1;
}

void usun_z_vectora(std::vector<kolejka_info> & kolejka, int id_procesu){
	kolejka.erase(
			std::remove_if(kolejka.begin(), kolejka.end(), [&](kolejka_info const & miejsce_w_kolejce){
				return miejsce_w_kolejce.id == id_procesu;
				}),
			kolejka.end());
}


void usun_z_vectora_wstazek(std::vector<kolejka_info_wstazki> & kolejka, int id_procesu){
	kolejka.erase(
			std::remove_if(kolejka.begin(), kolejka.end(), [&](kolejka_info_wstazki const & miejsce_w_kolejce){
				return miejsce_w_kolejce.id == id_procesu;
				}),
			kolejka.end());
}
			

void *receive_loop_skrzat(void * arg) {
    int msg[3]; //msg[0] - timestamp msg[1] - ? 
    int msgS[3]; 
    while(true){
        
        MPI_Status status;
        
        MPI_Recv(msg, 3, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

	//printf("Odebralem wiadomosc od %d. Moj numer to %d", status.MPI_SOURCE, rank);
        
        switch(status.MPI_TAG){

            case REQ_KONIE:

            	zwieksz_lamporta(msg[1]);

                msgS[0] = rank;
		msgS[1] = zegar;
                msgS[2] = 10; //placeholder

                printf("Lamport: %d . Odebralem REQ_KONIE od %d Moj nr to %d \n",msg[1], status.MPI_SOURCE, rank );
            		
                kolejka_info temp_kolejka;
                temp_kolejka.lamport = msg[1];
                temp_kolejka.id = status.MPI_SOURCE;

                pthread_mutex_lock(&kolejka_mutex);
                kolejka.push_back(temp_kolejka);
                sort(kolejka.begin(), kolejka.end(), por);
                pthread_mutex_unlock(&kolejka_mutex);


		for(const kolejka_info &k : kolejka){
			std::cout<<"Kolejka: proces"<< k.id << " Moj numer procesu "<<rank<<"\n";
		}

                pthread_mutex_lock(&wysylanie_mutex);
                MPI_Send(&msgS, 3, MPI_INT, status.MPI_SOURCE, ACK_ZGODA, MPI_COMM_WORLD);
		pthread_mutex_unlock(&wysylanie_mutex);

		break;

            case REQ_WSTAZKI:

                msgS[0] = rank;
		msgS[1] = zegar;
                msgS[2] = 10; //placeholder

                zwieksz_lamporta(msg[1]);
                printf("Lamport: %d . Odebralem REQ_WSTAZKI od %d Moj nr to %d \n",msg[1], status.MPI_SOURCE, rank );
            		
                kolejka_info_wstazki temp_kolejka_wstazki;
                temp_kolejka_wstazki.lamport = msg[1];
                temp_kolejka_wstazki.id = status.MPI_SOURCE;
                temp_kolejka_wstazki.wstazki = msg[2];

                pthread_mutex_lock(&kolejka_ze_wstazkami_mutex);
                kolejka_ze_wstazkami.push_back(temp_kolejka_wstazki);
                sort(kolejka_ze_wstazkami.begin(), kolejka_ze_wstazkami.end(), por_ze_wstazkami);
                pthread_mutex_unlock(&kolejka_ze_wstazkami_mutex);

                pthread_mutex_lock(&wysylanie_mutex);
                MPI_Send(&msgS, 3, MPI_INT, status.MPI_SOURCE, ACK_ZGODA, MPI_COMM_WORLD);
                pthread_mutex_unlock(&wysylanie_mutex);

		break;

            case ACK_ZGODA:

                pthread_mutex_lock(&zgody_mutex);
                zgody +=1;
                pthread_mutex_unlock(&zgody_mutex);

                printf("Lamport: %d. Moj Lamport: %d.Odebralem ACK_ZGODA od %d Moj nr: %d \n",msg[1],zegar ,status.MPI_SOURCE, rank);

		break;
            /* To bedzie potrzebne jesli beda psycholozki
	    
	    case INFO_KONIE:
                pthread_mutex_lock(&konie_mutex);
                    kolejka.erase(kolejka.begin());
                pthread_mutex_unlock(&konie_mutex);


            case INFO_WSTAZKI:
            //TODO: usuwanie odpowiedniego procesu z kolejki,a nie pierwszego
                pthread_mutex_lock(&wstazki_mutex);
                kolejka_ze_wstazkami.erase(kolejka_ze_wstazkami.begin());
                wstazki += msg[1];
                pthread_mutex_unlock(&wstazki_mutex);
		*/
		
		case INFO_ZASOBY:
			 pthread_mutex_lock(&konie_mutex);
			 usun_z_vectora(kolejka, status.MPI_SOURCE);
	 		 pthread_mutex_unlock(&konie_mutex);

			pthread_mutex_lock(&wstazki_mutex);
			usun_z_vectora_wstazek(kolejka_ze_wstazkami, status.MPI_SOURCE);
       			wstazki+=msg[2];
			pthread_mutex_unlock(&wstazki_mutex);	

			printf("Odebralem INFO_ZASOBY od %d. Moj numer to %d. \n", status.MPI_SOURCE, rank);
			break;	
        }

    }

}


int main(int argc, char **argv)
{
    int provided;
MPI_Init_thread(&argc , &argv , MPI_THREAD_MULTIPLE, &provided);


MPI_Comm_rank( MPI_COMM_WORLD , &rank);
MPI_Comm_size( MPI_COMM_WORLD , &size);

srand(time(0));

int msg[3];

zegar = 100 + rank;

pthread_t watek_odbiorczy;
pthread_create(&watek_odbiorczy, NULL, receive_loop_skrzat, 0);

        while(true){
            stan = ZASPOKOJONY;
                sleep(rand() % MAX_CZAS);
            int wstazki_skrzata = rand() % W;
            zgody = 0;
            stan = CZEKA_NA_KONIA;

            pthread_mutex_lock(&lamport_mutex);
            msg[0] = rank;
            msg[1] = zegar;
            msg[2] = 10; //placeholder

            moje_zamowienie_koni.lamport = zegar;
            moje_zamowienie_koni.id = rank;
            pthread_mutex_unlock(&lamport_mutex);


        pthread_mutex_lock(&wysylanie_mutex);
        for(int i = 0; i< size; i++){
            MPI_Send(&msg, 3, MPI_INT, i, REQ_KONIE, MPI_COMM_WORLD);
            printf("Lamport: %d . Wysłałem REQ_KONIE do %d .  Moj nr:%d \n",zegar ,i,rank);
        }
        pthread_mutex_unlock(&wysylanie_mutex);

        while(zgody!=size){//or?
            //uwu
        }

	printf("Uzyskalem zgody na konie. Moj numer to %d. \n",rank);


        while (ile_jest_przedemna(kolejka, moje_zamowienie_koni) < KONIE)
        {
            /* code */
        }
        
        //TUTAJ BIERZE KONIA JAK JEST W DOBRYM MIEJSCU KOLEJKI
        //JESLI JEST W MIEJSCU KOLEJKI GDZIE MIEJSCE <= ILOSC MAX KONI
	printf("Mam konia. Moj numer to %d. \n", rank);

        zgody = 0;

        stan = CZEKA_NA_WSTAZKI;

        pthread_mutex_lock(&lamport_mutex);
        msg[0] = rank;
        msg[1] = zegar;
        msg[2] = wstazki_skrzata;

        moje_zamowienie_wstazki.lamport = zegar;
        moje_zamowienie_wstazki.id = rank;
        moje_zamowienie_wstazki.wstazki = wstazki_skrzata;
        pthread_mutex_unlock(&lamport_mutex);


        pthread_mutex_lock(&wysylanie_mutex);
        for(int i = 0; i< size; i++){
            MPI_Send(&msg, 3, MPI_INT, i, REQ_WSTAZKI, MPI_COMM_WORLD);
            printf("Lamport: %d . Wysłałem REQ_WSTAZKI do %d .  Moj nr:%d \n",zegar ,i,rank);
        }
        pthread_mutex_unlock(&wysylanie_mutex);

        while(zgody!=size ){//or?
            //uwu
        }

	printf("Uzyskalem zgody na wstazki. Moj numer to %d. \n",rank);
        while (suma_wstazek(kolejka_ze_wstazkami,moje_zamowienie_wstazki) <= W)
        {
            /* code */
        }
        printf("Mam wstazki. Moj numer to %d. \n", rank);

        //TUTAJ BIERZE WSTAZKI
        //JESLI SUMA WSTAZEK JEGO I OSOB W KOLEJCE PRZED NIM JEST <= MAX ILOSCI WSTAZEK
    
        stan = ZAPLATA;
        sleep(rand() % MAX_CZAS);

	pthread_mutex_lock(&wysylanie_mutex);
	for(int i =0; i<size; i++){
		MPI_Send(&msg, 3, MPI_INT, i, INFO_ZASOBY, MPI_COMM_WORLD);
		printf("Lamport: %d . Wysłałem INFO_ZASOBY do %d . Moj nr:%d \n", zegar, i, rank); 
	}
	pthread_mutex_unlock(&wysylanie_mutex);
       // pthread_mutex_lock(&kolejka_mutex);
        
        //std::remove(kolejka.begin(), kolejka.end(), moje_zamowienie_koni);
        //kolejka.erase( , kolejka.end());
        //pthread_mutex_unlock(&kolejka_mutex);

       // pthread_mutex_lock(&kolejka_ze_wstazkami_mutex);
        //kolejka_ze_wstazkami.erase(std::remove(kolejka_ze_wstazkami.begin(), kolejka_ze_wstazkami.end(), moje_zamowienie_wstazki), kolejka_ze_wstazkami.end());
        //pthread_mutex_unlock(&kolejka_ze_wstazkami_mutex);

        }
}
