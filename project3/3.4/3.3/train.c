/* Basdanis Dionisios 2166
 * Antoniou Theodoros 2208
 * Tautoxronos programmatismos - 3 seira ergasion
 *  3.4.3 - train in louna park
 * */
#include <stdlib.h>
#include<stdio.h>
#include<pthread.h>
#include<unistd.h>
#include "ccr_macros.c"

#define SIZE 4

CCR_DECLARE(train_s)
CCR_DECLARE(psngs_q)
CCR_DECLARE(in_the_train)
CCR_DECLARE(mtx)

volatile int pw=0,p_counter=0,flag_train_s=0,flag_psngs_q=0,flag_in_the_train=0,flag_mtx=1;

void train_f(void *args_t);
void psngs_f(void *args_p);

int main(int argc,char *argv[]){
	int i,passengers;
	pthread_t *thr_p;
	pthread_t thr_t;
	
	CCR_INIT(train_s)
	CCR_INIT(psngs_q)
	CCR_INIT(in_the_train)
	CCR_INIT(mtx)
	
	
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
		
		CCR_EXEC(psngs_q,1,flag_psngs_q=1;)
		
		CCR_EXEC(train_s,flag_train_s==1, flag_train_s=0;)
		
		sleep(1);
		printf("End. Get out.\n");
		CCR_EXEC(mtx,flag_mtx==1, flag_mtx=0;)
		
		printf("Passenger out.\n");
		p_counter--;
		CCR_EXEC(in_the_train,1,flag_in_the_train=1;)
		CCR_EXEC(mtx,flag_mtx==1, flag_mtx=0;)
		
		
		CCR_EXEC(mtx,1,flag_mtx=1;)
		printf("Train empty. Waiting for new passengers.\n");

	}
}

void psngs_f(void *args_p){

	CCR_EXEC(psngs_q,flag_psngs_q==1, flag_psngs_q=0;)
	
	p_counter++;
	printf("Passenger in.\n");
	if(p_counter==SIZE){
		printf("Train starts with %d passengers.\n",p_counter);
		CCR_EXEC(train_s,1,flag_train_s=1;)
	}
	else{
		CCR_EXEC(psngs_q,1,flag_psngs_q=1;)
	}

	
	CCR_EXEC(in_the_train,flag_in_the_train==1, flag_in_the_train=0;)
	
	if(p_counter>0){
		printf("Passenger out.\n");		
		p_counter--;
		CCR_EXEC(in_the_train,1,flag_in_the_train=1;)
	}
	else{
		CCR_EXEC(mtx,1,flag_mtx=1;)
	}
}
