/* Basdanis Dionisios 2166
 * Antoniou Theodoros 2208
 * Tautoxronos programmatismos - 3 seira ergasion
 * 3.1 
*/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include <pthread.h>
#include "mandelCore.h"

#define WinW 300
#define WinH 300
#define ZoomStepFactor 0.5
#define ZoomIterationFactor 2
#define YES 1
#define NO 0


static Display *dsp = NULL;
static unsigned long curC;
static Window win;
static GC gc;

/* basic win management rountines */

static void openDisplay() {
  if (dsp == NULL) { 
    dsp = XOpenDisplay(NULL); 
  } 
}

static void closeDisplay() {
  if (dsp != NULL) { 
    XCloseDisplay(dsp); 
    dsp=NULL;
  }
}

void openWin(const char *title, int width, int height) {
  unsigned long blackC,whiteC;
  XSizeHints sh;
  XEvent evt;
  long evtmsk;

  whiteC = WhitePixel(dsp, DefaultScreen(dsp));
  blackC = BlackPixel(dsp, DefaultScreen(dsp));
  curC = blackC;
 
  win = XCreateSimpleWindow(dsp, DefaultRootWindow(dsp), 0, 0, WinW, WinH, 0, blackC, whiteC);

  sh.flags=PSize|PMinSize|PMaxSize;
  sh.width=sh.min_width=sh.max_width=WinW;
  sh.height=sh.min_height=sh.max_height=WinH;
  XSetStandardProperties(dsp, win, title, title, None, NULL, 0, &sh);

  XSelectInput(dsp, win, StructureNotifyMask|KeyPressMask);
  XMapWindow(dsp, win);
  do {
    XWindowEvent(dsp, win, StructureNotifyMask, &evt);
  } while (evt.type != MapNotify);

  gc = XCreateGC(dsp, win, 0, NULL);

}

void closeWin() {
  XFreeGC(dsp, gc);
  XUnmapWindow(dsp, win);
  XDestroyWindow(dsp, win);
}

void flushDrawOps() {
  XFlush(dsp);
}

void clearWin() {
  XSetForeground(dsp, gc, WhitePixel(dsp, DefaultScreen(dsp)));
  XFillRectangle(dsp, win, gc, 0, 0, WinW, WinH);
  flushDrawOps();
  XSetForeground(dsp, gc, curC);
}

void drawPoint(int x, int y) {
  XDrawPoint(dsp, win, gc, x, WinH-y);
  flushDrawOps();
}

void getMouseCoords(int *x, int *y) {
  XEvent evt;

  XSelectInput(dsp, win, ButtonPressMask);
  do {
    XNextEvent(dsp, &evt);
  } while (evt.type != ButtonPress);
  *x=evt.xbutton.x; *y=evt.xbutton.y;
}

/* color stuff */

void setColor(char *name) {
  XColor clr1,clr2;

  if (!XAllocNamedColor(dsp, DefaultColormap(dsp, DefaultScreen(dsp)), name, &clr1, &clr2)) {
    printf("failed\n"); return;
  }
  XSetForeground(dsp, gc, clr1.pixel);
  curC = clr1.pixel;
}

char *pickColor(int v, int maxIterations) {
  static char cname[128];

  if (v == maxIterations) {
    return("black");
  }
  else {
    sprintf(cname,"rgb:%x/%x/%x",v%64,v%128,v%256);
    return(cname);
  }
}

typedef struct{
	int iteration;
	int maxIterations_thr;
	int *res_thr;
	int n;
	mandel_Pars *slices_thr;
	pthread_cond_t *new_job;
}thread_arguments;

void thread_func(void *args);
volatile int  sum = 0, which_ready = 0,wrks_count = 0,flag=0,main_drawing=0,ready_workers=0,main_waiting=0,worker_ready=0;


pthread_mutex_t mtx_init = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_newjob = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_workers_finish = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_drawing = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx_wait_workers = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t init;
pthread_cond_t test;
pthread_cond_t drawing;
pthread_cond_t wait_workers;
pthread_cond_t wake_main;

int main(int argc, char *argv[]) {
  mandel_Pars pars,*slices;
  int i,j,x,y,nofslices,maxIterations,level,*res;
  int xoff,yoff,value;
  long double reEnd,imEnd,reCenter,imCenter;
  pthread_t *thr;
  thread_arguments args;
  

  
  printf("\n");
  printf("This program starts by drawing the default Mandelbrot region\n");
  printf("When done, you can click with the mouse on an area of interest\n");
  printf("and the program will automatically zoom around this point\n");
  printf("\n");
  printf("Press enter to continue\n");
  getchar();

  pars.reSteps = WinW; /* never changes */
  pars.imSteps = WinH; /* never changes */
 
  /* default mandelbrot region */

  pars.reBeg = (long double) -2.0;
  reEnd = (long double) 1.0;
  pars.imBeg = (long double) -1.5;
  imEnd = (long double) 1.5;
  pars.reInc = (reEnd - pars.reBeg) / pars.reSteps;
  pars.imInc = (imEnd - pars.imBeg) / pars.imSteps;

  printf("enter max iterations (50): ");
  scanf("%d",&maxIterations);
  printf("enter no of slices: ");
  scanf("%d",&nofslices);
  
  
  /* adjust slices to divide win height */

  while (WinH % nofslices != 0) { nofslices++;}

  /* allocate slice parameter and result arrays */
  
  slices = (mandel_Pars *) malloc(sizeof(mandel_Pars)*nofslices);
  res = (int *) malloc(sizeof(int)*pars.reSteps*pars.imSteps);
 
  /* open window for drawing results */

  openDisplay();
  openWin(argv[0], WinW, WinH);

  level = 1;

  thr = (pthread_t*)malloc(sizeof(pthread_t)*nofslices);
  if(thr == NULL){
	printf("error in allocation memory\n");
	return 1;
  }
  
  args.new_job = (pthread_cond_t*)malloc(sizeof(pthread_cond_t)*nofslices);


  args.n = nofslices;
  
   for (i=0; i<nofslices; i++) {
	pthread_mutex_lock(&mtx_init);  
	
	args.iteration = i;
	value = pthread_create(&thr[i], NULL, (void*)thread_func ,(void *)&args);
	if(value){
		fprintf(stderr,"Error creating thread\n");
		return (1);
	}	
	printf("(%d) thread created.\n",i);
	pthread_cond_wait(&init,&mtx_init);
	pthread_mutex_unlock(&mtx_init);
  }

  
  
  while (1) {
    clearWin();

    mandel_Slice(&pars,nofslices,slices);
	args.maxIterations_thr = maxIterations;
	args.slices_thr = slices;
	args.res_thr = res;

	sum=0;
	
	pthread_mutex_lock(&mtx_workers_finish);
	if(flag==0){
		pthread_cond_wait(&wake_main,&mtx_workers_finish);
	}
	pthread_mutex_unlock(&mtx_workers_finish);
	
	for(i=0;i<nofslices;i++){
		pthread_mutex_lock(&mtx_newjob);
		pthread_cond_signal(&test);
		pthread_mutex_unlock(&mtx_newjob);
	}
	

	y=0;
	do{ 
		pthread_mutex_lock(&mtx_wait_workers);
		if(worker_ready==NO){
			
			main_waiting = YES;
			pthread_cond_wait(&wait_workers,&mtx_wait_workers);
		}
		worker_ready = NO;
		pthread_mutex_unlock(&mtx_wait_workers);
		main_waiting = NO;
		
		
		
		y=which_ready*(slices[which_ready].imSteps);
		printf("(%d) thread returned results. Let's draw.\n",which_ready);
			for (j=0; j<slices[which_ready].imSteps; j++) {
				for (x=0; x<slices[which_ready].reSteps; x++) {
					setColor(pickColor(res[y*slices[which_ready].reSteps+x],maxIterations));
					drawPoint(x,y);
				}
				y++;
			}
			    
		sum++;
		
		pthread_mutex_lock(&mtx_drawing);
		ready_workers--;
		if(ready_workers>0){
			pthread_cond_signal(&drawing);
		}
		
		pthread_mutex_unlock(&mtx_drawing);
		
		main_drawing = NO;

	}while(sum<nofslices);

    /* get next focus/zoom point */
    
    getMouseCoords(&x,&y);
    xoff = x;
    yoff = WinH-y;
    
    /* adjust region and zoom factor  */
    
    reCenter = pars.reBeg + xoff*pars.reInc;
    imCenter = pars.imBeg + yoff*pars.imInc;
    pars.reInc = pars.reInc*ZoomStepFactor;
    pars.imInc = pars.imInc*ZoomStepFactor;
    pars.reBeg = reCenter - (WinW/2)*pars.reInc;
    pars.imBeg = imCenter - (WinH/2)*pars.imInc;
    
    maxIterations = maxIterations*ZoomIterationFactor;
    level++;
  } 
  
  /* never reach this point; for cosmetic reasons */

  free(slices);
  free(res);

  closeWin();
  closeDisplay();

}

void thread_func(void *args){
	int iteration,j;
	thread_arguments *arguments;
	mandel_Pars *slices;
	int *res;
	
	pthread_mutex_lock(&mtx_init);  
	arguments = (thread_arguments*)args;
	iteration = arguments->iteration;
	pthread_cond_signal(&init);
	pthread_mutex_unlock(&mtx_init);
	
	
	while(1){
		pthread_mutex_lock(&mtx_newjob);
		wrks_count++;
		flag=0;
		if(wrks_count == arguments->n){
			pthread_mutex_lock(&mtx_workers_finish);
			printf("last worker: waik up main for signal workers for new job\n");
			wrks_count=0;
			pthread_cond_signal(&wake_main);
			flag=1;
			pthread_mutex_unlock(&mtx_workers_finish);
		}

		pthread_cond_wait(&test,&mtx_newjob);
		printf("%d new job (waked up from main)\n",iteration);
		
		pthread_mutex_unlock(&mtx_newjob);

		
		
		slices = arguments->slices_thr;
		res = arguments->res_thr;
		
		mandel_Calc(&slices[iteration],arguments->maxIterations_thr,&res[iteration*slices[iteration].imSteps*slices[iteration].reSteps]);
		
		pthread_mutex_lock(&mtx_drawing);
		ready_workers++;
		if(main_drawing==YES){
			pthread_cond_wait(&drawing,&mtx_drawing);
		}
		pthread_mutex_unlock(&mtx_drawing);
		
		which_ready = iteration;
		main_drawing = YES;
		
		
		pthread_mutex_lock(&mtx_wait_workers);
		worker_ready = YES;
		if(main_waiting==YES){
			pthread_cond_signal(&wait_workers);
		}
		pthread_mutex_unlock(&mtx_wait_workers);
	}
}
