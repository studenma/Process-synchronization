/*
 * Soubor:  proj2.c
 * Datum:   2016/4/28
 * Autor:   Martin Studeny, xstude23@fit.vutbr.cz
 * Projekt  projekt do predmetu IOS
 * Popis:   Ukolem bylo implementovat modifikovany synchronizacni problem Roller Coaster
*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/syscall.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <limits.h>
#include <signal.h>
#include <time.h>

#define CHYBA "Neocekavany vstup\n"
#define CHYBA2 "Chyba ve funkci fork\n"
#define sem_t_SIZE sizeof(sem_t)

typedef struct parametry {
	int P; // pocet procesu reprezentujicich pasazera
	int C;	// kapacita voziku
	int PT; // maximalni hodnota doby, po ktere je genervoan novy proces pro pasazera (v milisekundach)
	int RT;// maximalni hodnota doby prujezdu trati ( v milisekundach)
} Tparam;


// deklarace funkci
int kontrolaArgumentu(Tparam *par);
void signal_handler();

/*
* globalni promenne
*/
sem_t *vystup, *nastoupil, *nastup, *jedem, *vystoupeno, *print, *finished, *vozikDOjel;
int *shm, *order, *order2;
FILE *soubor;

/*************************************
**************************************/
int main(int argc, char *argv[])
{
	// definovani signal handleru
	signal(SIGUSR1, signal_handler);

	srand((unsigned int)time(NULL));

	// deklarace promennych 
	Tparam param;
	pid_t ID1, ID2, ID3;
	int generated_p = 0;
	char *ptr0 = NULL;
	char *ptr1 = NULL;
	char *ptr2 = NULL;
	char *ptr3 = NULL;
	int wait_time1;
	int wait_time2;

	// inicializace promennych
	if(argc == 5)
	{
		param.P = strtol(argv[1], &ptr0, 10);
		param.C = strtol(argv[2], &ptr1, 10);
		param.PT = strtol(argv[3], &ptr2, 10);
		param.RT = strtol(argv[4], &ptr3, 10);
	}
	else
	{
		fprintf(stderr, CHYBA);
		return 1;
	}
	if(param.PT != 0 && param.RT != 0)
	{
		wait_time1 = (rand()%param.PT)*1000;
		wait_time2 = (rand()%param.RT)*1000;
	}
	else
	{
		wait_time1 = 0;
		wait_time2 = 0;
	}

	/*
	* kontrola vstupu
	*/
	int chyba = kontrolaArgumentu(&param);
	if(chyba == 1)
	{
		fprintf(stderr, CHYBA);
		return 1;
	}
	if(*ptr0 != 0 || *ptr1 != 0 || *ptr2 != 0 || *ptr3 != 0)
	{
		fprintf(stderr, CHYBA);
		return 1;
	}

	/*
	* Deklarace sdilene pameti
	*/
	shm = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	order = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	order2 = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	/*
	* Inicializace sdilene pameti
	*/
    *shm = 0;
    *order = 1;
    *order2 = 1;
    /*
    * Deklarace semaforu
    */
    vystup = mmap(NULL, sem_t_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
 	nastoupil = mmap(NULL, sem_t_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
 	nastup = mmap(NULL, sem_t_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
 	jedem = mmap(NULL, sem_t_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
 	vystoupeno = mmap(NULL, sem_t_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
 	print = mmap(NULL, sem_t_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
 	finished = mmap(NULL, sem_t_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	vozikDOjel = mmap(NULL, sem_t_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	/*
	* Inicializace semaforu
	*/
    sem_init(vystup, 1, 0);
    sem_init(nastoupil, 1, 0);
    sem_init(nastup, 1, 0);    
    sem_init(jedem, 1, 0);    
    sem_init(vystoupeno, 1, 0);    
    sem_init(print, 1, 1);    
    sem_init(finished, 1, 0);    
    sem_init(vozikDOjel, 1, 0);

    // otevreni souboru, do ktereho budeme zapisovat
    soubor = fopen("proj2.out", "w");
    setvbuf(soubor, NULL, _IONBF, 0);

	int passaID = 0;

    // vytvori se generator pasazeru
	if((ID1 = fork()) < 0)
	{
		fprintf(stderr, CHYBA2);
		kill(0, SIGUSR1);
	}
	if(ID1 == 0) // pokud jsme v generatoru pasazeru
	{
		while(generated_p++ != param.P) // pokud jeste nebylo vygenerovano dostatek pasazeru
		{
			usleep(wait_time1);
			if((ID2 = fork()) < 0)
			{
				fprintf(stderr, CHYBA2);
				kill(0, SIGUSR1);
			}
			passaID++;
			if(ID2 == 0) 
			{
				sem_wait(print);
					fprintf(soubor, "%d   : P %d   : started\n", ++(*shm), passaID);
				sem_post(print);

				sem_wait(nastup);

				sem_wait(print);
					*order+=1;
				
					fprintf(soubor, "%d   : P %d   : board\n", ++(*shm), passaID);

				if((*order) == param.C)
				{
					fprintf(soubor, "%d   : P %d   : board order last\n", ++(*shm), passaID);
					sem_post(nastoupil);
				}
				else
				{
						fprintf(soubor, "%d   : P %d   : board order %d\n", ++(*shm), passaID, *order);
				}

				sem_post(print);

				sem_wait(vystup);

				sem_wait(print);
					*order2+=1;


					fprintf(soubor, "%d   : P %d   : unboard\n", ++(*shm), passaID);
				

				if((*order2) == param.C)
				{
						fprintf(soubor, "%d   : P %d   : unboard order last\n", ++(*shm), passaID);
					sem_post(vystoupeno);
				}
				else
				{
						fprintf(soubor, "%d   : P %d   : unboard order %d\n", ++(*shm), passaID, *order2);

				}
				sem_post(print);				


				sem_wait(finished);
				sem_post(finished);

				sem_wait(print);
					fprintf(soubor, "%d   : P %d   : finished\n", ++(*shm), passaID);
				sem_post(print);
				exit(0);
			}
		}
		sem_wait(finished);
		sem_post(finished);
		exit(0);
	}
	else
	{
		if((ID3 = fork()) < 0)
		{
			fprintf(stderr, CHYBA2);
			kill(0, SIGUSR1);
		}
		if(ID3 == 0) // pokud jsme ve voziku
		{
			int carID = 1;

			// vytiskne start voziku
			sem_wait(print);
				fprintf(soubor, "%d   : C %d   : started\n", ++(*shm), carID);
			sem_post(print);
			
			/*
			* pocet iteraci je zavisly na poctu pasazeru a kapacite voziku
			* pocet cest
			* pr. 8 pasazeru, kapacita 4 --> cyklem se projde dvakrat
			*/
			for(int i = 0; param.P / param.C > i; i++)
			{

				/*
				* inicializujeme promenne zpatky na 0
				*/ 
				(*order) = 0;
				(*order2) = 0;

				// vytiskneme, ze vozik naklada pasazery
				sem_wait(print);
					fprintf(soubor, "%d   : C %d   : load\n", ++(*shm), carID);
				sem_post(print);

				/* 
				* cyklus se opakuje podle toho, kolik se do voziku vejde pasazeru
				*/
				for(int i = 0; i < param.C; i++) 
					sem_post(nastup);
				sem_wait(nastoupil);
				// vytiskneme, ze vozik se rozjel
				sem_wait(print);
					fprintf(soubor, "%d   : C %d   : run\n", ++(*shm), carID);
				sem_post(print);
				usleep(wait_time2);
				// vytiskneme, ze vozik vyklada pasazery
				sem_wait(print);
					fprintf(soubor, "%d   : C %d   : unload\n", ++(*shm), carID);
				sem_post(print);

				/*
				* pro kazdeho pasazera, ktery je ve voziku
				*/
				for(int i = 0; param.C > i; i++)
				{
					sem_post(vystup); // dame signal k vystupu pasazera
					
				}
				sem_wait(vystoupeno); // cekame na signal, ze predchozi pasazer uz vystoupil

			}
			// pro kazdeho pasazera opakujeme cyklus, ktery dava signal k ukonceni procesu

			sem_post(vozikDOjel);
			// vytiskneme, ze vozik skoncil
			sem_wait(print);
				fprintf(soubor, "%d   : C %d   : finished\n", ++(*shm), carID);
			sem_post(print);
			exit(0); // proces skonci
		}
	}
	sem_wait(vozikDOjel);
	//for(int i = 0; param.P > i; i++)
		sem_post(finished);


 	// zrusime zdroje
    sem_destroy(vystup);
    sem_destroy(nastoupil);
    sem_destroy(nastup);
    sem_destroy(jedem);
    sem_destroy(vystoupeno);
    sem_destroy(print);
    sem_destroy(finished);
    sem_destroy(vozikDOjel);

    munmap(vystup, sem_t_SIZE);
   	munmap(nastoupil, sem_t_SIZE);
    munmap(nastup, sem_t_SIZE);
    munmap(jedem, sem_t_SIZE);
 	munmap(vystoupeno, sem_t_SIZE);
 	munmap(print, sem_t_SIZE);
 	munmap(finished, sem_t_SIZE);
 	munmap(vozikDOjel, sem_t_SIZE);

 	munmap(shm, sizeof(int));
 	munmap(order, sizeof(int));
 	munmap(order2, sizeof(int));

 	//zavreme soubor
 	fclose(soubor);

	return 0;
}

/*************************************
**************************************/

int kontrolaArgumentu(Tparam *par)
{
	// pokud je zadany pocet pasazeru mensi nebo roven 0
	if(par->P <= 0)
		return 1;
	// pokud je zadany pocet kapacity voziku mensi nebo roven 0
	if(par->C <= 0)
		return 1;
	// pocet zadanych pasazeru musi byt vetsi nez pocet zadane kapacity voziku
	if(par->P <= par->C)
		return 1;
	// zadani pasazeri museji byt vzdy nasobkem zadane kapacity voziku
	if((par->P % par->C) != 0)
		return 1;
	// hodnota doby, po ktere je generovan novy proces pro pasazera musi byt v rozmezi 0 a 5000
	if(par->PT < 0 || par->PT > 5000)
		return 1;
	// doba prujezdu trati musi byt v rozmezi 0 a 5000
	if(par->RT < 0 || par->RT > 5000)
		return 1;
	// pokud je pocet argumentu ruzny od 5
	return 0;
}

void signal_handler()
{
    sem_destroy(vystup);
    sem_destroy(nastoupil);
    sem_destroy(nastup);
    sem_destroy(jedem);
    sem_destroy(vystoupeno);
    sem_destroy(print);
    sem_destroy(finished);
    sem_destroy(vozikDOjel);

    munmap(vystup, sem_t_SIZE);
   	munmap(nastoupil, sem_t_SIZE);
    munmap(nastup, sem_t_SIZE);
    munmap(jedem, sem_t_SIZE);
 	munmap(vystoupeno, sem_t_SIZE);
 	munmap(print, sem_t_SIZE);
 	munmap(finished, sem_t_SIZE);
 	munmap(vozikDOjel, sem_t_SIZE);

 	munmap(shm, sizeof(int));
 	munmap(order, sizeof(int));
 	munmap(order2, sizeof(int));

 	fclose(soubor);
 	exit(2);
}
