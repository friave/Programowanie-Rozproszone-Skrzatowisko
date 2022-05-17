#include <pthread.h>
#include <vector>
#include <thread>
#include <chrono>
#include <ctime>
#include <iostream>
#include <cstdlib>

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
pthread_mutex_t konie_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t wstazki_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t pacjenci_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t salki_mutex = PTHREAD_MUTEX_INITIALIZER;

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

int stan;

std::vector<kolejka_info> kolejka = {};  //dla skrzatow to kolejka o konie, a dla psycholozek o salki
std::vector<kolejka_info_wstazki> kolejka_ze_wstazkami = {}; // dla skrzatow to kolejka o wstazki, a dla psycholozek o pacjentow

void zwieksz_lamporta(int z){
    pthread_mutex_lock(&lamport_mutex);
    zegar=max(z,zegar)+1;
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
                printf("Lamport: %d . Odebrałem REQ_KONIE od %d Moj nr to %d \n",msg[0], status.MPI_SOURCE, rank );
            		
                pthread_mutex_lock(&kolejka_mutex);
                kolejka.push_back(temp_queue);
                sort(kolejka.begin(), kolejka.end(), my_compare);
                pthread_mutex_unlock(&kolejka_mutex);

            case REQ_WSTAZKI:

            case ACK_ZGODA:

            case INFO_KONIE:

            case INFO_WSTAZKI:

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




    MPI_Status status;
    MPI_Comm_rank( MPI_Comm comm , &rank);
    MPI_Comm_size( MPI_Comm comm , &size);

    srand(time(0));

    zegar = 100 + rank;


    if((float)rank <= size/2) //skrzat
    {
        stan = ZASPOKOJONY;
            std::sleep_for(rand() % MAX_CZAS);
        int wstazki_skrzata = rand() % W;
        stan = CZEKA_NA_KONIA;
        
        }

        



    }
    else //psycholozka
    {
        stan = NA_PRZERWIE;
        std::sleep_for(rand() % MAX_CZAS);
        stan = CZEKA_NA_PACJENTA;
    }
}
