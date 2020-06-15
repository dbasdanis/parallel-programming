/* Basdanis Dionisios 2166
 * Antoniou Theodoros 2208
 * Tautoxronos programmatismos - 3 seira ergasion
 *  3.4 
 * */
typedef struct {
    int q0_c;
	int q1_c;
	int q2_c;
	pthread_mutex_t mtx;
	pthread_cond_t q0;
	pthread_cond_t q1;
	pthread_cond_t q2;
} ccr;

#define CCR_DECLARE(label) ccr label;

#define CCR_INIT(label) { pthread_mutex_init(&label.mtx,NULL);        \
                          label.q1_c = 0;                             \
	                      label.q2_c = 0;                             \
						  label.q0_c = 0;                             \
}

#define CCR_EXEC(label,cond,body) {                           \
		pthread_mutex_lock(&label.mtx);                       \
		if(label.q2_c>0){                                     \
			label.q0_c++;                                     \
			pthread_cond_wait(&label.q0,&label.mtx);		  \
		}								                      \
		while(!(cond)){                                       \
			label.q1_c++;                                     \
			if(label.q2_c>0){                                 \
				label.q2_c--;								  \
				pthread_cond_signal(&label.q2);               \
			}		                                          \
			else if(label.q0_c>0) {                           \
				label.q0_c--;								  \
				pthread_cond_signal(&label.q0);               \
			}												  \
			pthread_cond_wait(&label.q1,&label.mtx);          \
															  \
			if(label.q1_c>0){                                 \
				label.q1_c--;                                 \
				pthread_cond_signal(&label.q1);               \
				label.q2_c++;                                 \
				pthread_cond_wait(&label.q2,&label.mtx);      \
			}	                                              \
			else if(label.q2_c>0){                            \
				pthread_cond_signal(&label.q2);               \
				pthread_cond_wait(&label.q2,&label.mtx);      \
			}                                                 \
		}													  \
															  \
        body                                                  \
															  \
        if(label.q1_c>0){                                     \
            label.q1_c--;									  \
            pthread_cond_signal(&label.q1);                   \
        }                                                     \
        else if(label.q2_c>0){                                \
            label.q2_c--;									  \
            pthread_cond_signal(&label.q2);                   \
        }                                                     \
        else if(label.q0_c>0){                                \
            label.q0_c--;									  \
            pthread_cond_signal(&label.q0);                   \
        }                                                     \
        pthread_mutex_unlock(&label.mtx);                     \
}
