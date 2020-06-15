/* Basdanis Dionisios 2166
 * Antoniou Theodoros 2208
 * Tautoxronos programmatismos - 3 seira ergasion
 *  3.2 - train in louna park
 * */
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>


#define SIZE 4
#define YES 1
#define NO 0

pthread_mutex_t mtx_c;
pthread_cond_t train_wt;
pthread_cond_t psng_q;
pthread_cond_t in_the_train;
pthread_cond_t m;

volatile int p_counter=0,flag_tr_wt = YES,get_on_train = NO,psngs_waiting = 0,fl=0;

void train_f(void *args_t);
void psngs_f(void *args_p);

int main(int argc,char *argv[]){
	int i,passengers;
	pthread_t *thr_p;
	pthread_t thr_t;
	
	if(argc != 2){
		printf("Wrong number of arguments\n");
		return 1;
	}
	
	passengers = atoi(argv[1]);
	thr_p = (pthread_t *)malloc(passengers*sizeof(pthread_t));
	if(thr_p == NULL){
		printf("Error on allocation memory\n");
		return 1;
	}
	
	pthread_create(&thr_t,NULL,(void*)train_f,NULL);
	printf("***Train thread created.***\n");
	
	for(i=0;i<passengers;i++){
		pthread_create(&thr_p[i],NULL,(void *)psngs_f,NULL);
		printf("***(%d)Passenger thread created.***\n",i);
	}
	pthread_join(thr_t,NULL);
	for(i=0;i<passengers;i++){
		pthread_join(thr_p[i],NULL);
	}
	
	return 0;
}

void train_f(void *args_t){
	
	while(1){
		
		pthread_mutex_lock(&mtx_c);
		if(psngs_waiting>0){
			pthread_cond_signal(&psng_q);
		}
		psngs_waiting--;
		get_on_train = YES;
 	    pthread_mutex_unlock(&mtx_c);
		
		flag_tr_wt = YES;
 	    pthread_mutex_lock(&mtx_c);
		if(flag_tr_wt == YES){
			pthread_cond_wait(&train_wt,&mtx_c);
		}
		
 		pthread_mutex_unlock(&mtx_c);
		
		sleep(1);
		printf("End. Get out.\n");
		

		printf("Passenger out.\n");
		p_counter--;
		
		pthread_mutex_lock(&mtx_c);
		pthread_cond_signal(&in_the_train);
		
		if(fl==0){
			pthread_cond_wait(&m,&mtx_c);
		}
		fl=0;
		pthread_mutex_unlock(&mtx_c);
		
		printf("Train empty. Waiting for new passengers.\n");
		
	}
}

void psngs_f(void *args_p){
	
	pthread_mutex_lock(&mtx_c);
	if(get_on_train==NO){
		psngs_waiting++;
		pthread_cond_wait(&psng_q,&mtx_c);
	}
	p_counter++;
	printf("Passenger in.\n");
	
	
	
	if(p_counter==SIZE){
		printf("Train starts with %d passengers.\n",p_counter);
		flag_tr_wt = NO;
		get_on_train=NO;
		pthread_cond_signal(&train_wt);
		pthread_mutex_unlock(&mtx_c);
	}
	else{

		pthread_cond_signal(&psng_q);
 		pthread_mutex_unlock(&mtx_c);
	}
	
	
	pthread_cond_wait(&in_the_train,&mtx_c);
	pthread_mutex_unlock(&mtx_c);
	
	if(p_counter>0){
		printf("Passenger out.\n");		
		p_counter--;
		pthread_mutex_lock(&mtx_c);
		pthread_cond_signal(&in_the_train);
		pthread_mutex_unlock(&mtx_c);
	}
	else{
		fl=1;
		pthread_cond_signal(&m);
	}
}
