/* Basdanis Dionisios 2166
 * Antoniou Theodoros 2208
 * Tautoxronos programmatismos - 3 seira ergasion
 *  3.2 - cars on a bridge
 * */
#include <pthread.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>

#define RED 0
#define BLUE 1
#define N 20
#define LIMIT 10
#define MAXCARS 5

typedef struct {
	char colour;
	int time;	
}car_args;

void cars(void* car);


pthread_mutex_t mtx_init = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t init;
pthread_cond_t blq;
pthread_cond_t rdq;

volatile int reds=0,blues=0,rw=0,bw=0,counter=0;

int main(int argc, char* argv[]){
    int i=0,res;
	pthread_t thr[N];
	car_args arguments[N];

	while(i<N){
		pthread_mutex_lock(&mtx_init);
		scanf(" %c",&(arguments[i].colour));
		scanf(" %d",&(arguments[i].time));
		
		pthread_create(&thr[i],NULL,(void*)cars,(void*)&arguments[i]);
		i++;
		pthread_cond_wait(&init,&mtx_init);
		pthread_mutex_unlock(&mtx_init);
		
	}
	
	for(i=0;i<N;i++){
		res = pthread_join(thr[i],NULL);
		if(res !=0){
			printf("\npthread_join failed\n");
			return 1;
		}
	}
	
	return 0;
}

void cars(void* car_thr){
	car_args* car;
	
	
	pthread_mutex_lock(&mtx_init);  
	car = (car_args*)car_thr;
	pthread_cond_signal(&init);
	pthread_mutex_unlock(&mtx_init);
	
	pthread_mutex_lock(&mtx);
	if(car->colour == 'b'){
		if(reds>0 || (counter >= LIMIT && rw>0) || blues == MAXCARS){
			bw++;

			pthread_cond_wait(&blq,&mtx);
			pthread_mutex_unlock(&mtx);
			if(bw>0 && (counter<LIMIT-1 || rw==0) && blues < MAXCARS-1){
				bw--;
				blues++;
				pthread_cond_signal(&blq);
				pthread_mutex_unlock(&mtx);
			}
			else
				pthread_mutex_unlock(&mtx);
			counter++;
		}
		else{
			blues++;
			counter++;
			pthread_mutex_unlock(&mtx);
		}		
	}
	else if(car->colour == 'r'){
		if(blues>0 || (counter >= LIMIT && bw>0) || reds==MAXCARS){
			rw++;

			pthread_cond_wait(&rdq,&mtx);
			pthread_mutex_unlock(&mtx);
			if(rw>0 && (counter<LIMIT-1 || bw==0) && reds < MAXCARS-1){
				rw--;
				reds++;
				pthread_cond_signal(&rdq);
				pthread_mutex_unlock(&mtx);
			}
			else
				pthread_mutex_unlock(&mtx);
			counter++;
 			
		}
		else{
			reds++;
			counter++;
			pthread_mutex_unlock(&mtx);
		}		
		
		
	}
	
	printf("%ld: %c car on the bridge    \t%d blue    %d red cars on bridge.\n",pthread_self(),car->colour,blues,reds);
	/*start of bridge*/

	sleep(car->time);

	/*end of bridge */

	pthread_mutex_lock(&mtx);
	if(car->colour == 'b'){
		blues--;
		printf("%ld: %c car out of the bridge \t%d blue    %d red cars on bridge.\n",pthread_self(),car->colour,blues,reds);
		if(blues==0){
			 if(rw>0){
				rw--;
				reds++;
				counter=0;
				pthread_cond_signal(&rdq);
				pthread_mutex_unlock(&mtx);
			}
			else if(bw>0){
				bw--;
				blues++;
 				counter = 0;
				pthread_cond_signal(&blq);
				pthread_mutex_unlock(&mtx);
			}
		}
		else if(bw>0 && counter<LIMIT-1){
				bw--;
				blues++;
				pthread_cond_signal(&blq);
				pthread_mutex_unlock(&mtx);
		}
		else
		    pthread_mutex_unlock(&mtx);
		
	}
	else if(car->colour == 'r'){
		reds--;
		printf("%ld: %c car out of the bridge \t%d blue    %d red cars on bridge.\n",pthread_self(),car->colour,blues,reds);
		if(reds == 0){
			if(bw>0){
				bw--;
				blues++;
				counter=0;
				pthread_cond_signal(&blq);
				pthread_mutex_unlock(&mtx);
			}
			else if(rw>0){
				rw--;
				reds++;
 				counter =0;
				pthread_cond_signal(&rdq);
				pthread_mutex_unlock(&mtx);
			}
		}
		else if(rw>0 && counter<LIMIT-1){
			rw--;
			reds++;
			pthread_cond_signal(&rdq);
			pthread_mutex_unlock(&mtx);
		}
		else
			pthread_mutex_unlock(&mtx);
	}
}

