/* Basdanis Dionisios 2166
 * Antoniou Theodoros 2208
 * Tautoxronos programmatismos - 3 seira ergasion
 * 3.4.2 - cars on a bridge
 * */
#include <pthread.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include "ccr_macros.c"

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


volatile int reds=0,blues=0,rw=0,bw=0,counter=0,id,flag_blq=0,flag_rdq=0,flag_mtx=1,flag_mtx1=1;

CCR_DECLARE(blq)
CCR_DECLARE(rdq)
CCR_DECLARE(mtx)
CCR_DECLARE(mtx1)

int main(int argc, char* argv[]){
    int i=0,res;
	pthread_t thr[N];
	car_args arguments[N];
	
	CCR_INIT(blq)
	CCR_INIT(rdq)
	CCR_INIT(mtx)
	CCR_INIT(mtx1)

	while(i<N){
		CCR_EXEC(mtx1,flag_mtx1==1, )
		flag_mtx1=0;
		scanf(" %c",&(arguments[i].colour));
		scanf(" %d",&(arguments[i].time));
		
		pthread_create(&thr[i],NULL,(void*)cars,(void*)&arguments[i]);
		i++;
		
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
	
	car = (car_args*)car_thr;
	CCR_EXEC(mtx1,1,flag_mtx1=1;)
	
	CCR_EXEC(mtx,flag_mtx==1, flag_mtx=0;)
	
	if(car->colour == 'b'){
		if(reds>0 || (counter >= LIMIT && rw>0) || blues == MAXCARS){
			bw++;
			CCR_EXEC(mtx,1,flag_mtx=1;)
			CCR_EXEC(blq,flag_blq==1, flag_blq=0;)
			
			if(bw>0 && (counter<LIMIT-1 || rw==0) && blues < MAXCARS-1){
				bw--;
				blues++;
				CCR_EXEC(blq,1,flag_blq=1;)
			}
			else
				CCR_EXEC(mtx,1,flag_mtx=1;)
			counter++;
		}
		else{
			blues++;
			counter++;
			CCR_EXEC(mtx,1,flag_mtx=1;)
		}		
	}
	else if(car->colour == 'r'){
		if(blues>0 || (counter >= LIMIT && bw>0) || reds==MAXCARS){
			rw++;
			CCR_EXEC(mtx,1,flag_mtx=1;)
			CCR_EXEC(rdq,flag_rdq==1, flag_rdq=0;)
			
			if(rw>0 && (counter<LIMIT-1 || bw==0) && reds < MAXCARS-1){
				rw--;
				reds++;
				CCR_EXEC(rdq,1,flag_rdq=1;)
			}
			else
				CCR_EXEC(mtx,1,flag_mtx=1;)
			counter++;
 			
		}
		else{
			reds++;
			counter++;
			CCR_EXEC(mtx,1,flag_mtx=1;)
		}		
		
		
	}
	
	printf("%ld: %c car on the bridge    \t%d blue    %d red cars on bridge.\n",pthread_self(),car->colour,blues,reds);
	/*start of bridge*/

	sleep(car->time);

	/*end of bridge */
	CCR_EXEC(mtx,flag_mtx==1, flag_mtx=0;)
	
	
	if(car->colour == 'b'){
		blues--;
		printf("%ld: %c car out of the bridge \t%d blue    %d red cars on bridge.\n",pthread_self(),car->colour,blues,reds);
		if(blues==0){
			 if(rw>0){
				rw--;
				reds++;
				counter=0;
				CCR_EXEC(rdq,1,flag_rdq=1;)
			}
			else if(bw>0){
				bw--;
				blues++;
 				counter = 0;
				CCR_EXEC(blq,1,flag_blq=1;)
			}
		}
		else if(bw>0 && counter<LIMIT-1){
				bw--;
				blues++;
				CCR_EXEC(blq,1,flag_blq=1;)
		}
		else
			CCR_EXEC(mtx,1,flag_mtx=1;)
		
	}
	else if(car->colour == 'r'){
		reds--;
		printf("%ld: %c car out of the bridge \t%d blue    %d red cars on bridge.\n",pthread_self(),car->colour,blues,reds);
		if(reds == 0){
			if(bw>0){
				bw--;
				blues++;
				counter=0;
				CCR_EXEC(blq,1,flag_blq=1;)
			}
			else if(rw>0){
				rw--;
				reds++;
 				counter =0;
				CCR_EXEC(rdq,1,flag_rdq=1;)
			}
		}
		else if(rw>0 && counter<LIMIT-1){
			rw--;
			reds++;
			CCR_EXEC(rdq,1,flag_rdq=1;)
		}
		else
			CCR_EXEC(mtx,1,flag_mtx=1;)
	}
}

