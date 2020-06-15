/* Basdanis Dionisios 2166
 * Antoniou Theodoros 2208
 * Tautoxronos programmatismos - 2 seira ergasion
 * 2.1 - semaphores implementation using mutexes
*/
#include <pthread.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>

#define UP 1
#define DOWN 0

typedef struct bsemaphore{
	pthread_mutex_t mutex;
	pthread_mutexattr_t attr;
	int condition;
}my_bsem;

pthread_mutex_t  mutex1 = PTHREAD_MUTEX_INITIALIZER; // for mutual exclusion


void mybsem_init(my_bsem* sem, int value){
	int res;	
	
	res = pthread_mutexattr_init(&(sem->attr));
	if(res !=0){
		printf("\nmutexattr init failed\n");
		return;
	}
	res = pthread_mutexattr_settype(&(sem->attr), PTHREAD_MUTEX_NORMAL);
	if(res != 0){
		printf("\nmutexattr settype failed\n");
		return;
	}
	
	res = pthread_mutex_init(&(sem->mutex),&(sem->attr));
	if(res != 0){
		printf("\nmutex init failed\n");
		return;
	}
	
	
	if(value>1 || value<0){
 		printf("Binary semaphore: 0 or 1.\n");
		exit(1);
	}
		
	sem->condition = value;
	if(value==0)
		pthread_mutex_lock(&(sem->mutex)); //initialized 0
}

void mybsem_up(my_bsem* sem){
	
	pthread_mutex_lock(&mutex1);

	if(sem->condition == 1){
		printf("ERROR...\n");
	}
	else if(sem->condition == 0){
 		//printf("Unlocking...\n");
		sem->condition = 1;
		pthread_mutex_unlock(&(sem->mutex));
	}
	pthread_mutex_unlock(&mutex1);
	
}
void mybsem_down(my_bsem* sem){

	pthread_mutex_lock(&mutex1);
	
	if(sem->condition == 1){ 
		sem->condition = 0;
		pthread_mutex_lock(&(sem->mutex));
 		//printf("Locked first time.\n");
	}
	else if(sem->condition == 0){
		//printf("Already down...\n");
		pthread_mutex_unlock(&mutex1);
		pthread_mutex_lock(&(sem->mutex));
		sem->condition = 0;
 		//printf("Unlocked!!!\n");
        return;
	}
		
	pthread_mutex_unlock(&mutex1);
	
}

int mybsem_destroy(my_bsem* sem){
	
	if(sem->condition == 0)
		pthread_mutex_unlock(&(sem->mutex));
	
	if(pthread_mutexattr_destroy(&(sem->attr)) != 0){
		printf("\nmutexattr destroy1 failed\n");
		return 1;
	}
	if(pthread_mutex_destroy(&(sem->mutex)) != 0){
		printf("\nmutex destroy2 failed\n");
		return 1;
	}

	
	printf("\nSuccessful destroy.\n");
	
	return 0;
}
