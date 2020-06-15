/* Basdanis Dionisios 2166
 * Antoniou Theodoros 2208
 
   Parallel Programming
   
   The interpreter was implemented in the programming language C. In the main function the interface is done with the user, 
   that is, the user can press some of the commands run, list, kill and exit to do the corresponding actions. The interpretation 
   of the code becomes line by line. First we run the whole file and save in a table of structs (struct label) the name of the 
   label and how many bytes from the beginning of the file you find it. After storing all the labels we start interpreting the 
   code line by line. In the program we have the struct program which has as fields the various information that each application 
   needs, the sys_thread which is for each thread of the system, the number of which is determined by the user, through which we 
   run the various application threads, the command that serves to store the commands of the command, the var which is a struct 
   for the variables and the array that is for the tables in the program. user run some run and run the init_new_prog function 
   which passes as definitions the name of the application program and the n number of the thread in which it should run and 
   initializes the fields of the structs sys_thread and programm, opening the file with the code. Then run_prog starts running 
   in which some of the commands are interpreted as it also calls the fcommand function for the interpretation of the rest. 
   In addition to these functions, we also have the search_var search functions (search variable), search_label, search_array 
   (search the variables in the tables) and find_label (which finds and stores all the program labels in a table). We also 
   have the split_string and getStr functions which are used to break up a string and kill to terminate an application. Each
   new program runs on the next system thread. The alternation between the application threads is done every five commands or 
   if any of the DOWN, UP, SLEEP and RETURN commands are found.
*/

#include<stdio.h>
#include<string.h>
#include<unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<pthread.h>
#include <sched.h>
#include"my_sems.c"

#define NAME_SIZE 15
#define LINE_SIZE 100
#define ON 1
#define OFF 0
#define SLEEPING 2
#define BLOCKED 3
#define NULL_TERM '\0'

typedef struct{
	char *str;
	char *arg1;
	char *arg2;
	char *arg3;
}command;

typedef struct{
	char name[10];
	int position;
}label;

typedef struct{
    char name[NAME_SIZE];
    int val;
}var;

typedef struct{
    char name[NAME_SIZE];
    int *vals;
	int size;
}array;

typedef struct{
	int condition;
	double t_left;
    pthread_t t;
    char name[NAME_SIZE];
	int argc;
    int num_of_labels;
	int  *argv;
	label *labels;
	var *vars;
	int var_array_size;
	array *arrays;
	int num_of_arrays;
	FILE *fd;
}program;

typedef struct{
	pthread_t t;
	program *prog_array;
	int numof_progs;
	int new_prog_array;
	my_bsem wait_new_prog;
	my_bsem mtx;
}sys_thread;

my_bsem gen_mtx;

var *global_vars = NULL;
int globals_array_size = 0;

sys_thread *threads;
int num_of_thr;

int all_progs =0;

void init_new_prog(char *input, int thr);
int kill(int id);
command* split_string(char* str);
int search_array(char *arg, var *varray,int var_array_size,array *arrays,int ar_ar_size,int *pos);
void run_prog(void* prog_array);
void fcommand(int lsize,FILE *fd,label *label_array,command fcmd,var *vars,int vsize,array *arrays,int num_of_arrays);
int search_label(char *str,label *larray, int size);
int search_var(char*str,var *varray,int var_array_size);
label *find_labels(FILE* fd, int* size);

int main(int argc, char *argv[]){
	char buffer[LINE_SIZE]={'\0'},*temp;
	//program *prog_array = NULL;
	int i,res,y,counter=0;//,w;
	
	if(argc!=2){
		printf("Wrong number of arguments.\n");
		return 1;
	}
	else
		num_of_thr = atoi(argv[1]);
	
	mybsem_init(&gen_mtx,1);
	
	threads = (sys_thread*)malloc(sizeof(sys_thread)*num_of_thr);
	
	for(i=0;i<num_of_thr;i++){
		mybsem_init(&threads[i].wait_new_prog,0);
		mybsem_init(&threads[i].mtx,1);
		threads[i].numof_progs = 0;
		threads[i].new_prog_array = 0;
		threads[i].prog_array = NULL;
		res = pthread_create(&(threads[i].t),NULL,(void*)run_prog, (void*)(&threads[i]));
		if(res!=0){
			printf("Pthread create error.\n");
			return 1;
		}
		printf("Thread %d created.\n",i);
	}
	while(1){
		for(i=0;i<LINE_SIZE;i++){
			buffer[i] = '\0';
		}
		
		fgets(buffer,LINE_SIZE,stdin);
		
		buffer[strlen(buffer)-1] = '\0';
	
		temp = strtok(buffer," ");
		if(strcmp(temp,"run")==0){                            //run new programm
			temp = strtok(NULL," 	");
			if(temp==NULL){
				printf("Try again with correct program name.\n");
			}
			else{
				if(counter>=num_of_thr)
					counter =0;
				mybsem_down(&threads[counter].mtx);
				init_new_prog(temp,counter);
				if(threads[counter].numof_progs==1){
					mybsem_up(&threads[counter].wait_new_prog);
				}
				mybsem_up(&threads[counter].mtx);
				counter++;
            }
		}
		else if(strcmp(temp,"list")==0){      //list running programms
            
			printf("%c[4mID\t  Name\t\tCondition\tSystem Thread%c[0m\n\n",27,27);
			for(y=0;y<num_of_thr;y++){
				for(i=0;i<threads[y].numof_progs;i++){
					printf("%d \t%s  ",threads[y].prog_array[i].argv[0],threads[y].prog_array[i].name);
					switch(threads[y].prog_array[i].condition){
						case 1: printf("  \tRUNNING\t"); break;
						case 2: printf("  \tSLEEPING\t"); break; 
						case 3: printf("  \tBLOCKED\t"); break;
						case 0: printf("  \tDEAD\t"); break;
					}
					printf("(%d)%ld\n\n",y,threads[y].prog_array[i].t);
				}
			}
		}
		else if(strcmp(temp,"kill")==0){                      //kill a programm
			temp = strtok(NULL," \t\r\n");
			res = atoi(temp);
			
			int res1 = kill(res);

			if(res1==-1){
				printf("Error in kill.\n");
			}

			
		}
		else if(strcmp(temp,"exit")==0){                      //kill all and exit
         for(i=0;i<num_of_thr;i++){
			res = pthread_cancel(threads[i].t); 
			if(res){
				printf("Error in thread cancel %d.\n",i);
			}
		}
		 return 0;
		}
		else{
			printf("No such command.\n");
			
		}
	}
	return 0;
}

char *getStr(char *start, char *end){
    int length=end-start;
    int i=0;
    char *result = (char *)malloc (sizeof (char) * (length+1));

    while (i < length) {
    result[i] = *start;
    start++;
    i++;
    }

    result[i] = NULL_TERM;

    return result;
}

void init_new_prog(char *buffer, int thr){
	program *prog;
	int count=0,progs = 0;
	char *tag_read, *real_tag = "#PROGRAM";

	tag_read=(char*)malloc(sizeof(char)*LINE_SIZE);
	
// 	mybsem_down(&mtx);
	
	prog = threads[thr].prog_array;
	progs = threads[thr].numof_progs;
	
	
	prog = (program*)realloc(prog,sizeof(program)*(progs+1));
	strcpy(prog[progs].name,buffer);
	prog[progs].argc = 0;
	prog[progs].t = threads[thr].t;
	count = 0;
	
	count++;
	prog[progs].argv = (int*)malloc(sizeof(int));
	prog[progs].argv[0] = all_progs;

	while(buffer!=NULL){
		buffer = strtok(NULL," \0");
		if(buffer!=NULL){
			count++;
			prog[progs].argv = (int*)realloc(prog[progs].argv,count*sizeof(int));
			prog[progs].argv[count-1] = atoi(buffer);
		}
	}
	prog[progs].argc = count;
	
// 	printf("prog: %s\nargc: %d\n",prog[progs].name,prog[progs].argc);
// 	if((prog[progs].argc) > 0){
// 		for(int i=0;i<prog[progs].argc;i++){
// 			printf("argv[%d]:%d\n",i,prog[progs].argv[i]);
// 		}
// 		
// 	}

	prog[progs].fd = fopen(prog[progs].name,"r");
 	if(prog[progs].fd==NULL){
 		printf("open error.\n");
 		return ;
 	}
 	
 	
 	
 	fgets(tag_read,LINE_SIZE,prog[progs].fd);
// 	int y = strlen(tag_read);
	
//  	fseek(prog[progs].fd,0,SEEK_SET);
	
	tag_read = strtok(tag_read," \t\r\n");
	if(strcmp(real_tag,tag_read)!=0){
		printf("Wrong type of file.\n");
		return ;
	}
	
	fseek(prog[progs].fd,0,SEEK_SET);
    prog[progs].labels = find_labels(prog[progs].fd,&(prog[progs].num_of_labels));
    
// 	printf("**LABELS**\n");
//     for(i=0;i<prog[progs].num_of_labels;i++){
// 		printf("labels[%d].name: %s, labels[%d].position:%d\n",i,prog[progs].labels[i].name,i,prog[progs].labels[i].position);
// 	}
// 	printf("\n");
	//fgets(tag_read,LINE_SIZE,prog[progs].fd);
	
	prog[progs].var_array_size++;
	prog[progs].vars = (var*)realloc(prog[progs].vars,sizeof(var)*prog[progs].var_array_size);
	strcpy(prog[progs].vars[prog[progs].var_array_size-1].name,"$argc");
	prog[progs].vars[prog[progs].var_array_size-1].val =prog[progs].argc;
	
	prog[progs].num_of_arrays++;
	prog[progs].arrays = (array*)realloc(prog[progs].arrays,sizeof(array)*prog[progs].num_of_arrays);
	strcpy(prog[progs].arrays[prog[progs].num_of_arrays-1].name,"$argv");
	prog[progs].arrays[prog[progs].num_of_arrays-1].vals = prog[progs].argv;
	prog[progs].arrays[prog[progs].num_of_arrays-1].size = prog[progs].argc;
 	prog[progs].condition = ON;
 	fseek(prog[progs].fd,0,SEEK_SET);
	
	progs++;
	threads[thr].prog_array = prog;
	threads[thr].numof_progs = progs;
	all_progs ++;
	threads[thr].new_prog_array  = 1;
// 	mybsem_up(&mtx);
	
	
}

int kill(int id){
	int i,y,w,k;
	
	for(w=0;w<num_of_thr;w++){
		for(i=0;i<threads[w].numof_progs;i++){
			if(threads[w].prog_array[i].argv[0] == id){
				threads[w].prog_array[i].condition = OFF;
				k = w;
			}
		}
	}

	for(y=i;y<threads[k].numof_progs-i;y++){
		threads[k].prog_array[y] = threads[k].prog_array[y+1];
	}
	all_progs--;
	threads[k].numof_progs--;
	if(threads[k].numof_progs==0)
		threads[k].prog_array = NULL;
	
	return 0;
}

void fcommand(int lsize,FILE* fd,label *label_array,command fcmd,var *vars,int vsize,array *arrays,int num_of_arrays){
	int res=0,res1=0,res2=0,pos,pos1,value,value1;
	
	if(fcmd.arg1!=NULL){
		if(fcmd.arg1[strlen(fcmd.arg1)-1]=='\n'){
			fcmd.arg1[strlen(fcmd.arg1)-1]='\0';
		}
	}
	if(fcmd.arg2!=NULL){
		if(fcmd.arg2[strlen(fcmd.arg2)-1]=='\n'){
			fcmd.arg2[strlen(fcmd.arg2)-1]='\0';
		}
	}
	if(fcmd.arg3!=NULL){
		if(fcmd.arg3[strlen(fcmd.arg3)-1]=='\n'){
			fcmd.arg3[strlen(fcmd.arg3)-1]='\0';
		}
	}

// 	
	if(strcmp(fcmd.str,"LOAD") == 0){
		mybsem_down(&gen_mtx);
// 		printf("LOAD\n");
		res1 = search_var(fcmd.arg1,vars,vsize);
		res2 = search_array(fcmd.arg1,vars,vsize,arrays,num_of_arrays,&pos);
		
		res = search_var(fcmd.arg2,global_vars,globals_array_size); 
		if(res == -1){
			printf("Global variable does not exist.\n");
		}
		else{
			vars[res1].val = global_vars[res].val;
		}
		mybsem_up(&gen_mtx);
	}
	else if(strcmp(fcmd.str,"STORE") == 0){
// 		printf("STORE\n");
		res = search_var(fcmd.arg1,global_vars,globals_array_size);
		if(res == -1){
			printf("wrong name of variable\n");
		}
		if(fcmd.arg2[0]=='$'){
			res1 = search_var(fcmd.arg2,vars,vsize);
			if(res1 == -1){
				res1 = search_array(fcmd.arg2,vars,vsize,arrays,num_of_arrays,&pos);
				if(res1 == -1)
					printf("syntax error6\n");
				else
					res1 = arrays[res1].vals[pos];
			}
			else{
				res1 = vars[res1].val;
			}
		}
		else{
			res1 = atoi(fcmd.arg2);
		}
		global_vars[res].val = res1;
		mybsem_up(&gen_mtx);
	}
	else if(strcmp(fcmd.str,"SET") == 0){
// 		printf("SET\n");

		res = search_var(fcmd.arg1,vars,vsize);
		if(res == -1){
			res2 = search_array(fcmd.arg1,vars,vsize,arrays,num_of_arrays,&pos);
			if(res2 == -1)
				printf("wrong name of variable\n");
		}
		if(fcmd.arg2[0]=='$'){
			res1 = search_var(fcmd.arg2,vars,vsize);
			if(res1 == -1){
				res1= search_array(fcmd.arg2,vars,vsize,arrays,num_of_arrays,&pos1);
				if(res1 == -1)
					printf("syntax error5\n");
				else
					value = arrays[res1].vals[pos1];
			}
			else{
				value = vars[res1].val;
			}
		}
		else{
			value = atoi(fcmd.arg2);
		}
		if(res!=-1)
			vars[res].val = value;
		else if(res2!=-1)
			arrays[res2].vals[pos] = value;
		
	}
	else if((strcmp(fcmd.str,"ADD") == 0) || (strcmp(fcmd.str,"SUB") == 0)||(strcmp(fcmd.str,"MUL") == 0)||(strcmp(fcmd.str,"DIV") == 0)||(strcmp(fcmd.str,"MOD") == 0)){
		if(fcmd.arg1[0]=='$'){
			res = search_var(fcmd.arg1,vars,vsize);
			if(res == -1){
				res2= search_array(fcmd.arg1,vars,vsize,arrays,num_of_arrays,&pos);
				if(res2 == -1)
					printf("wrong name of variable\n");
			}
		}
		if(fcmd.arg2[0]=='$'){
			res1 = search_var(fcmd.arg2,vars,vsize);
			if(res1 == -1){
				res1= search_array(fcmd.arg2,vars,vsize,arrays,num_of_arrays,&pos1);
				if(res1 == -1)
					printf("syntax error4\n");
				else
					value = arrays[res1].vals[pos1];
			}
			else{
				value = vars[res1].val;
			}
		}
		else{
			value = atoi(fcmd.arg2);
		}
		
		if(fcmd.arg3[0]=='$'){
			res1 = search_var(fcmd.arg3,vars,vsize);
			if(res1 == -1){
				res1= search_array(fcmd.arg3,vars,vsize,arrays,num_of_arrays,&pos1);
				if(res1 == -1)
					printf("syntax error3\n");
				else
					value1 = arrays[res1].vals[pos1];
			}
			else{
				value1 = vars[res1].val;
			}
		}
		else{
			value1 = atoi(fcmd.arg3);
		}
		
		if((strcmp(fcmd.str,"ADD") == 0)){
// 			printf("ADD\n");
			if(res!=-1)
				vars[res].val = value + value1;
			else if(res2!=-1)
				arrays[res2].vals[pos] = value + value1;
		}
		else if((strcmp(fcmd.str,"SUB") == 0)){
// 			printf("SUB\n");
			if(res!=-1)
				vars[res].val = value - value1;
			else if(res2!=-1)
				arrays[res2].vals[pos] = value - value1;
		}
		else if(strcmp(fcmd.str,"MUL") == 0){
// 			printf("MUL\n");
			if(res!=-1)
				vars[res].val = value * value1;
			else if(res2!=-1)
				arrays[res2].vals[pos] = value * value1;
		}
		else if(strcmp(fcmd.str,"DIV") == 0){
// 			printf("DIV\n");
			if(res!=-1)
				vars[res].val = value / value1;
			else if(res2!=-1)
				arrays[res2].vals[pos] = value / value1;
		}
		else if(strcmp(fcmd.str,"MOD") == 0){
// 			printf("MOD\n");
			if(res!=-1)
				vars[res].val = value % value1;
			else if(res2!=-1)
				arrays[res2].vals[pos] = value % value1;
		}
	}
	else if((strcmp(fcmd.str,"BRGT") == 0) || (strcmp(fcmd.str,"BRGE") == 0) || (strcmp(fcmd.str,"BRLT") == 0) || (strcmp(fcmd.str,"BRLE") == 0)||(strcmp(fcmd.str,"BREQ") == 0)){
		
		if(fcmd.arg1[0]=='$'){
			res1 = search_var(fcmd.arg1,vars,vsize);
			if(res1 == -1){
				res1= search_array(fcmd.arg1,vars,vsize,arrays,num_of_arrays,&pos);
				if(res1 == -1)
					printf("syntax error2\n");
				else
					res1 = arrays[res1].vals[pos];
			}
			else{
				res1 = vars[res1].val;
			}
		}
		else{
			res1 = atoi(fcmd.arg2);
		}
		if(fcmd.arg2[0]=='$'){
			res2 = search_var(fcmd.arg2,vars,vsize);
			if(res2 == -1){
				res2= search_array(fcmd.arg2,vars,vsize,arrays,num_of_arrays,&pos1);
				if(res2 == -1)
					printf("syntax error1\n");
				else
					res2 = arrays[res2].vals[pos1];
			}
			else{
				res2 = vars[res2].val;
			}
		}
		else{
			res2 = atoi(fcmd.arg2);
		}
		if(strcmp(fcmd.str,"BRGT") == 0){
// 			printf("BRGT\n");
			if(res1 > res2){
				res = search_label(fcmd.arg3,label_array,lsize);
				if(res == -1){
					printf(" This label does not exist!!!\n");
				}
				fseek(fd,res,SEEK_SET);
			}
		}
		if(strcmp(fcmd.str,"BRGE") == 0){
// 			printf("BRGE\n");
			if(res1 >= res2){
				res = search_label(fcmd.arg3,label_array,lsize);
				if(res == -1){
					printf(" This label does not exist!!!\n");
				}
				fseek(fd,res,SEEK_SET);
			}
		}
		if(strcmp(fcmd.str,"BRLT") == 0){
// 			printf("BRLT\n");
			if(res1 < res2){
				res = search_label(fcmd.arg3,label_array,lsize);
				if(res == -1){
					printf(" This label does not exist!!!\n");
				}
				fseek(fd,res,SEEK_SET);
			}
		}
		if(strcmp(fcmd.str,"BRLE") == 0){
// 			printf("BRLE\n");
			if(res1 <= res2){
				res = search_label(fcmd.arg3,label_array,lsize);
				if(res == -1){
					printf(" This label does not exist!!!\n");
				}
				fseek(fd,res,SEEK_SET);
			}
		}
		if(strcmp(fcmd.str,"BREQ") == 0){
// 			printf("BREQ\n");
			if(res1 == res2){
				res = search_label(fcmd.arg3,label_array,lsize);
				if(res == -1){
					printf(" This label does not exist!!!\n");
				}
				fseek(fd,res,SEEK_SET);
			}
		}
	}
	else if(strcmp(fcmd.str,"BRA") == 0){
		res = search_label(fcmd.arg1,label_array,lsize);
		if(res == -1){
			printf(" This label does not exist!!!\n");
		}
		fseek(fd,res,SEEK_SET);
	}
	else if(strcmp(fcmd.str,"DOWN") == 0){
// 		printf("DOWN\n");
	}
	else if(strcmp(fcmd.str,"UP") == 0){
// 		printf("UP\n");
	}
	else {
		if(strcmp(fcmd.str,"#PROGRAM")!=0 /*&& strcmp(fcmd.str,"RETURN")!=0*/)
			printf("Wrong command\n");
//         strcpy(fcmd.str,"RETURN");
	}
}


int search_label(char *str, label *larray, int size){
	int i,label_pos;
	
	if(str[strlen(str)-1]=='\n'){
		str[strlen(str)-1]='\0';
	}
	
	
	if(larray!=NULL){
		for(i=0;i<size;i++){
			if(strcmp(larray[i].name,str)==0){
				label_pos = larray[i].position;
				return label_pos;
			}
		}
	}
	return -1;
}

int search_var(char *str,var *varray,int var_array_size){
	int i;
	
	if(varray != NULL){
		for(i=0;i<var_array_size;i++){
			if(strcmp(varray[i].name,str)==0){
				return i;
			}
		}
	}
	return -1;
}

int search_array(char *arg, var *varray,int var_array_size,array *arrays,int ar_ar_size,int *pos){
	int i,res,val;
	char *tmp = NULL,*name;
	
	tmp = (char*)malloc(sizeof(char)*20);
    name = (char*)malloc(sizeof(char)*20);
	strcpy(name,arg);
	
	if(arg[strlen(arg)-1] == ']'){
		tmp = strtok(arg,"[");
		strcpy(name,tmp);
		tmp = strtok(NULL,"[]");
		if(tmp[0] == '$'){
			res = search_var(tmp,varray,var_array_size);
			if(res==-1){
				printf("Wrong variable.\n");
			}
			else{
				val = varray[res].val;
			}
		}
		else
			val = atoi(tmp);
	}
	
	
	if(arrays != NULL){
		for(i=0;i<ar_ar_size;i++){
			if(strcmp(arrays[i].name,name)==0){
				*pos = val;
				return i;
			}
		}
	}
	return -1;
}

label *find_labels(FILE* fd,int* size){
    char buffer[LINE_SIZE],*tmp;
    label *labels_array = NULL;
    int y=0,bytes_read =0,startofline = 0;
    
    do{
       fgets(buffer,LINE_SIZE,fd);
       startofline = bytes_read;
       bytes_read += strlen(buffer);
       tmp = strtok(buffer," 	");
       if(tmp[0]=='L' && strcmp(tmp,"LOAD")!=0){
		   y++;
           labels_array = (label*)realloc(labels_array,y*sizeof(label));
           strcpy(labels_array[y-1].name,tmp);
           labels_array[y-1].position = startofline;
       }
    }while(feof(fd)==0);
    *size = y;
    return labels_array;
    
}

void run_prog(void* thread){
	char *buffer,*str1,*tmp,*str2,*start,*end;
	int  i,val,val1,k=0,res,val2,useless,counter=0,w;
	command *cmd; 
	sys_thread *thr;
    program *prog;
	int pc,y,offset;
	char *token_list[10] = {NULL};
	char reject = ' ';
	

	thr = (sys_thread*)thread;
	
	mybsem_down(&thr->wait_new_prog);
	
	prog = thr->prog_array;
	
	buffer = (char*)malloc(sizeof(char)*LINE_SIZE);
	pc=0;
     while(1){   
		mybsem_down(&thr->mtx);
		if(thr->new_prog_array==1){
			prog = thr->prog_array;
			thr->new_prog_array = 0;
		}
		
		if(counter % 5 == 0 || strcmp(cmd->str,"UP")==0 || strcmp(cmd->str,"LOAD")==0 || strcmp(cmd->str,"DOWN")==0 || strcmp(cmd->str,"RETURN")==0 || strcmp(cmd->str,"SLEEP")==0){
				
			
			pc++;
			if(pc >= thr->numof_progs){
				pc = 0;
			}
			//int p=0;
			while(prog[pc].condition==SLEEPING){
				mybsem_up(&thr->mtx);
				//sched_yield();

				prog[pc].t_left = prog[pc].t_left - 0.0001;
				
				if(prog[pc].t_left <= 0){
					prog[pc].condition=ON;
				}
				
				pc++;
				mybsem_down(&thr->mtx);
				if(thr->new_prog_array==1){
					prog = thr->prog_array;
					thr->new_prog_array = 0;
				}
				if(pc>=thr->numof_progs){
					pc = 0;
				}
			}
			counter = 0;
			while(prog[pc].condition==BLOCKED){
				pc++;
				if(pc>=thr->numof_progs){
					pc = 0;
				}
			}		
		}
		
		if(thr->numof_progs==0){
			mybsem_up(&thr->mtx);
			mybsem_down(&thr->wait_new_prog);
			mybsem_down(&thr->mtx);
			pc=0;
			counter = 0;
			prog = thr->prog_array;
			thr->new_prog_array = 0;
		}
	
		for(w=0;w<num_of_thr;w++){
			for(y=0;y<threads[w].numof_progs;y++){
				if(threads[w].prog_array[y].condition == SLEEPING){
					threads[w].prog_array[y].t_left =threads[w].prog_array[y].t_left - 0.0001;
					if(threads[w].prog_array[y].t_left == 0){
						threads[w].prog_array[y].condition = ON;
					}
				}
			}
		}
				
		tmp = (char*)malloc(sizeof(char)*LINE_SIZE);
		str2 = (char*)malloc(sizeof(char)*LINE_SIZE);
        for(i=0;i<LINE_SIZE;i++){
			buffer[i] = '\0';
			tmp[i] = '\0';
			str2[i] = '\0';
		}
		

		if(thr->new_prog_array==1){
			prog = thr->prog_array;
			thr->new_prog_array = 0;
		}
		
		fgets(buffer,LINE_SIZE,prog[pc].fd);
		offset = strlen(buffer);
		if(buffer[strlen(buffer)-1] == '\n')
			buffer[strlen(buffer)-1] = '\0';
		
		strcpy(tmp,buffer);
		strcpy(str2,buffer);
		
		cmd = split_string(buffer);
	
		//printf("%d",pc);
		if(cmd->str!=NULL){
			if(strcmp(cmd->str,"RETURN")==0){ 
// 				mybsem_down(&mtx);
// 				printf("RETURN\n");
				prog[pc].condition = OFF;
				res = kill(prog[pc].argv[0]);
				if(res == -1){
					printf("Error in kill.\n");
				}
// 				mybsem_up(&mtx);
			}
			else if(strcmp(cmd->str,"PRINT")==0){
				printf("%d ",prog[pc].argv[0]);
				str1 = buffer;
				while(*str1!='"'){
					str1++;
					tmp++;
				}
				str1++;
				tmp++;
				do{
					if(*str1=='\0')
						putchar(' ');
					printf("%c",*str1);
					str1++;
					tmp++;
				}while(*str1!='"');
				str1++;
				tmp++;
				
				start = tmp;
				end = tmp;
				i = 0;
                    
//                 for(i=0;i<)
                char *ptr = (char*)malloc(LINE_SIZE);
                strcpy(ptr,tmp);
                ptr = strtok(ptr,"\n\r\t ");
                if(ptr!=NULL){
                
                    while((end = strchr(start,(int)reject))){
                        token_list[i] =NULL;
                        token_list[i]=getStr(start,end);
                            
                        i++;
                        end++;
                        start = end;
                    }
                    token_list[i]=start;
                        
                    for(i=0;i<5;i++){
                        if(token_list[i]!=NULL && token_list[i][0]>= '0' && token_list[i][0]<= '9'){
                            printf(" %d",atoi(token_list[i]));
                        }
                    }

                    
                    k=1;
                    while(k<5 && token_list[k]!=NULL){
                        res = search_var(token_list[k],prog[pc].vars,prog[pc].var_array_size);
                        if(res == -1){
                            res = search_array(token_list[k],prog[pc].vars,prog[pc].var_array_size,prog[pc].arrays,prog[pc].num_of_arrays,&useless);
                            if(res != -1){
                                printf(" %d",prog[pc].arrays[res].vals[useless]);
                            }
                        }
                        else{
                            printf(" %d",prog[pc].vars[res].val);
                        }
                        k++;
                    }
                }
				
				printf("\n");
				
			}
			else if(strcmp(cmd->str,"SLEEP") == 0){
// 				printf("SLEEP\n");
				if(cmd->arg1[0]=='$'){
					res = search_var(cmd->arg1,prog[pc].vars,prog[pc].var_array_size);
					if(res == -1){
						res= search_array(cmd->arg1,prog[pc].vars,prog[pc].var_array_size,prog[pc].arrays,prog[pc].num_of_arrays,&useless);
						if(res == -1)
							printf("syntax error in sleep\n");
						else
							res = prog[pc].arrays[res].vals[useless];
					}
					else{
						res = prog[pc].vars[res].val;
					}
				}
				else{
					res = atoi(cmd->arg1);
				}
		// 		sleep(res);
// 				printf("sleeping for %d.\n",res);
				prog[pc].t_left = res*200;
				prog[pc].condition = SLEEPING;

			}
			
			else if(strcmp(cmd->str,"UP")==0 || strcmp(cmd->str,"DOWN")==0){
				mybsem_down(&gen_mtx);
				res = search_var(cmd->arg1,global_vars,globals_array_size);
				if(res==-1){
					printf("Semaphore has not been initialized.\n");
					//break;
				}
				if(strcmp(cmd->str,"UP")==0){
// 					printf("UP\n");
					global_vars[res].val++;

					for(w=0;w<num_of_thr;w++){
						for(y=0;y<threads[w].numof_progs;y++){
							if(threads[w].prog_array[y].condition == BLOCKED){
								threads[w].prog_array[y].condition = ON;
							}
						}
					}
				}
				else if(strcmp(cmd->str,"DOWN")==0){
// 					printf("DOWN\n");
					if(global_vars[res].val == 0){
						fseek(prog[pc].fd,-offset,SEEK_CUR);
						prog[pc].condition = BLOCKED;
					}
					else
						global_vars[res].val--;
				}
				mybsem_up(&gen_mtx);
			}
			else{
		
				if(cmd->arg1!=NULL){
					if(cmd->arg1[0] == '$'){ 	//an exoume kapoia metavlhth
						if(strcmp(cmd->str,"STORE")==0){
							mybsem_down(&gen_mtx);
							val = search_var(cmd->arg1,global_vars,globals_array_size);
							if(val == -1){
								globals_array_size++;
								global_vars = (var*)realloc(global_vars,sizeof(var)*globals_array_size);
								strcpy(global_vars[globals_array_size-1].name,cmd->arg1);
							}
						}
						else if(cmd->arg1[strlen(cmd->arg1)-1] == ']'){
							strcpy(tmp,cmd->arg1);
							tmp = strtok(tmp,"[");
							res= search_array(tmp,prog[pc].vars,prog[pc].var_array_size,prog[pc].arrays,prog[pc].num_of_arrays,&useless);
							if(res==-1){
								prog[pc].num_of_arrays++;
								prog[pc].arrays = (array*)realloc(prog[pc].arrays,sizeof(array)*prog[pc].num_of_arrays);
								strcpy(prog[pc].arrays[prog[pc].num_of_arrays-1].name,tmp);
								tmp = strtok(NULL,"[]");
								val2 = atoi(tmp);
								prog[pc].arrays[prog[pc].num_of_arrays-1].size = val2+1; 
								prog[pc].arrays[prog[pc].num_of_arrays-1].vals = (int*)realloc(prog[pc].arrays[prog[pc].num_of_arrays-1].vals,sizeof(int)*prog[pc].arrays[prog[pc].num_of_arrays-1].size);
							}
							else{
								tmp = strtok(NULL,"[]");
								val2 = atoi(tmp);
								if(val2+1 > prog[pc].arrays[prog[pc].num_of_arrays-1].size){
									prog[pc].arrays[prog[pc].num_of_arrays-1].size = val2+1; 
									prog[pc].arrays[prog[pc].num_of_arrays-1].vals = (int*)realloc(prog[pc].arrays[prog[pc].num_of_arrays-1].vals,sizeof(int)*prog[pc].arrays[prog[pc].num_of_arrays-1].size);
								}
							}
		                    
						}
						else{
							val1 = search_var(cmd->arg1,prog[pc].vars,prog[pc].var_array_size);
							if(val1 == -1){			//den uparxei ston pinaka twn metavlhtwn
								prog[pc].var_array_size++;
								prog[pc].vars = (var*)realloc(prog[pc].vars,sizeof(var)*(prog[pc].var_array_size));
								strcpy(prog[pc].vars[prog[pc].var_array_size-1].name,cmd->arg1);
							}
							
						}
					}
				}
				fcommand(prog[pc].num_of_labels,prog[pc].fd,prog[pc].labels,*cmd,prog[pc].vars,prog[pc].var_array_size,prog[pc].arrays,prog[pc].num_of_arrays);
			}
		}
		counter++;
		mybsem_up(&thr->mtx);
		sched_yield();
 	}
	
	mybsem_down(&thr->mtx);
	for(pc=0;pc<thr->numof_progs;pc++){
		printf("%d\n",pc);
		printf("\nLOCAL VARIABLES\n");
		for(i=0;i<prog[pc].var_array_size;i++){
			printf("vars[%d].name: %s | val:%d\n",i,prog[pc].vars[i].name,prog[pc].vars[i].val);
		}
		
		printf("\nGLOBAL VARIABLES\n");
		for(i=0;i<globals_array_size;i++){
			printf("vars[%d].name: %s | val:%d\n",i,global_vars[i].name,global_vars[i].val);
		}
		
		printf("\nARRAYS\n");
		for(i=0;i<prog[pc].num_of_arrays;i++){
			printf("arrays[%d].name: %s ",i,prog[pc].arrays[i].name);
			for(y=0;y<prog[pc].arrays[i].size;y++){
				printf("%d ",prog[pc].arrays[i].vals[y]);
			}
			printf("\n");
		}
	}
	mybsem_up(&thr->mtx);
	
	prog[pc].condition = OFF;
}

command* split_string(char* str){
    char *tmp = NULL , *t1, *t2,*t3, *t4 ,*t5,bad[] = "\t \r\n";
	command *cmd;
    
    cmd = (command*)malloc(sizeof(command));	
	
// 	printf("str:%s.\n",str);
	
	t1 = NULL;
    t2 = NULL;
	t3 = NULL;
    t4 = NULL;
    t5 = NULL;
	
    tmp = strtok(str,bad);
    t1 = tmp;

	tmp = strtok(NULL,bad);
    if(tmp!=NULL){
           t2 = tmp;
        tmp = strtok(NULL,bad);
        if(tmp!=NULL){
               t3= tmp;
            tmp = strtok(NULL,bad);
            if(tmp!=NULL){
                  t4 = tmp;
                tmp = strtok(NULL,bad);
                if(tmp!=NULL){
                       t5 = tmp;
				}
			}
		}
	}    

	if(t1[0]=='L' && strcmp(t1,"LOAD")!=0){
		cmd->str = t2;
		cmd->arg1 = t3;
		cmd->arg2 = t4;
		cmd->arg3 = t5;
	}
	else{
		cmd->str = t1;
		cmd->arg1 = t2;
		cmd->arg2 = t3;
		cmd->arg3 = t4;
	}
	
	return cmd;
    
}




