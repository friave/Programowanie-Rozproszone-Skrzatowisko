#include <pthread.h>
#include <vector>
#include <thread>
#include <chrono>
#include <ctime>
#include <iostream>
#include <cstdlib>
#include <algorithm>
//#include <mpi.h>


#define W = 30
#define KONIE = 8 
#define S = 6
#define MAX_CZAS = 10

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
#define INFO_SALKI 5003s


pthread_mutex_t lamport_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t kolejka_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t kolejka_ze_wstazkami_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t konie_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wstazki_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pacjenci_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t salki_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t zgody_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wysylanie_mutex = PTHREAD_MUTEX_INITIALIZER;



typedef struct kolejka_info{
  	int lamport;
  	int id;     
} kolejka_info;

typedef struct kolejka_info_wstazki{
  	int lamport;
  	int id;
  	int wstazki;
} kolejka_info_wstazki;

int zegar;

int size,rank;

int konie;
int wstazki;
int salki;

int zgody;

int stan;



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

void *receive_loop_skrzat(void * arg) {
    int msg[2]; //msg[0] - timestamp msg[1] - ? 
    int msgS[2]; 
    while(true){
        
        MPI_Status status;
        
        MPI_Recv(msg, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
        switch(status.MPI_TAG){

            case REQ_KONIE:

            	zwieksz_lamporta(msg[0]);

                msgS[0] = zegar;
                msgS[1] = rank;

                printf("Lamport: %d . Odebrałem REQ_KONIE od %d Moj nr to %d \n",msg[0], status.MPI_SOURCE, rank );
            		
                kolejka_info temp_kolejka;
                temp_kolejka.lamport = msg[0];
                temp_kolejka.id = status.MPI_SOURCE;

                pthread_mutex_lock(&kolejka_mutex);
                kolejka.push_back(temp_kolejka);
                sort(kolejka.begin(), kolejka.end(), por);
                pthread_mutex_unlock(&kolejka_mutex);

                pthread_mutex_lock(&wysylanie_mutex);
                MPI_Send(&msgS, 2, MPI_INT, status.MPI_SOURCE, ACK_ZGODA, MPI_COMM_WORLD);
                pthread_mutex_unlock(&wysylanie_mutex);

            case REQ_WSTAZKI:

                msgS[0] = 0;
                msgS[1] = 0;

                zwieksz_lamporta(msg[0]);
                printf("Lamport: %d . Odebrałem REQ_KONIE od %d Moj nr to %d \n",msg[0], status.MPI_SOURCE, rank );
            		
                kolejka_info_wstazki temp_kolejka_wstazki;
                temp_kolejka_wstazki.lamport = msg[0];
                temp_kolejka_wstazki.id = status.MPI_SOURCE;
                temp_kolejka_wstazki.wstazki = msg[1];

                pthread_mutex_lock(&kolejka_ze_wstazkami_mutex);
                kolejka_ze_wstazkami.push_back(temp_kolejka_wstazki);
                sort(kolejka_ze_wstazkami.begin(), kolejka_ze_wstazkami.end(), por);
                pthread_mutex_unlock(&kolejka_ze_wstazkami_mutex);

                pthread_mutex_lock(&wysylanie_mutex);
                MPI_Send(&msgS, 2, MPI_INT, status.MPI_SOURCE, ACK_ZGODA, MPI_COMM_WORLD);
                pthread_mutex_unlock(&wysylanie_mutex);

            case ACK_ZGODA:

                pthread_mutex_lock(&zgody_mutex);
                zgody +=1;
                pthread_mutex_unlock(&zgody_mutex);

                printf("Lamport: %d. My Lamport: %d.Odebrałem ACK_ZGODA od %d Moj nr: %d \n",msg[0],zegar ,status.MPI_SOURCE, rank);

            case INFO_KONIE:
                pthread_mutex_lock(&konie_mutex);
                    kolejka.erase(kolejka.begin());
                pthread_mutex_unlock(&konie_mutex);


            case INFO_WSTAZKI:
                pthread_mutex_lock(&wstazki_mutex);
                kolejka_ze_wstazkami.erase(kolejka_ze_wstazkami.begin());
                wstazki += msg[1];
                pthread_mutex_unlock(&wstazki_mutex);
        }

    }

}


void *receive_loop_psycholozka(void * arg) {
    int msg[2]; //msg[0] - timestamp msg[1] - ? 
    int msgS[2]; 
    while(true){
        
        MPI_Status status;
        
        MPI_Recv(msg, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        
        switch(status.MPI_TAG){

            case REQ_PACJENCI:

            case REQ_SALKI:



            case ACK_ZGODA:

            case INFO_PACJENCI:

            case INFO_SALKI:
        }

    }

}


int main(int argc, char **argv)
{
    MPI_Init_thread(&argc , &argv , MPI_THREAD_MULTIPLE, &provided);




    MPI_Comm_rank( MPI_Comm comm , &rank);
    MPI_Comm_size( MPI_Comm comm , &size);

    srand(time(0));

    int msg[2];

    zegar = 100 + rank;

    if((float)rank <= size/2) //skrzat
    {
        while(true){
            stan = ZASPOKOJONY;
                std::sleep_for(rand() % MAX_CZAS);
            int wstazki_skrzata = rand() % W;
            zgody = 0;
            stan = CZEKA_NA_KONIA;
            msg[0] = zegar;
            msg[1] = 10; //placeholder

            pthread_mutex_lock(&wysylanie_mutex);
            for(int i = 0; i<= size/2; i++){
                MPI_Send(&msg, 2, MPI_INT, i, REQ_KONIE, MPI_COMM_WORLD);
                printf("Lamport: %d . Wysłałem REQ_KONIE do %d .  Moj nr:%d \n",zegar ,i,rank);
            }
            pthread_mutex_unlock(&wysylanie_mutex);

            while(zgody<size/2 || ){
                //uwu
            }




            
            }
    }

        



    }
    else //psycholozka
    {
        while(true){
            stan = NA_PRZERWIE;
            std::sleep_for(rand() % MAX_CZAS);
            stan = CZEKA_NA_PACJENTA;
        }
    }
}