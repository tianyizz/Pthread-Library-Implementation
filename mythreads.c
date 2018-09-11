/*
	Tianyi Zhang
	C17587650
	CPSC 3220

*/

#include "mythreads.h"
#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <assert.h>


#define START_SIZE 1024

//different thread status
#define NOT_EXIT 0
#define WAIT 1
#define CURRENT 2
#define DONE 3

//structure with thread context, thread status, and a pointer to store the result
typedef struct contextStore{
	ucontext_t* th;
	int status;
	void* resultH;
}cs;

int totalThread=1;
int lastIndex=1;
int curSize=START_SIZE;

//structural pointer array to keep tracks of the malloced pointer
cs** ptrC=NULL;

//lock array (1 means locked, 0 mean unlock)
int*status_lock=NULL;

//condition variable array
int**conditions_lock=NULL;

//interrupts settup
int interruptsAreDisabled;

static void interruptDisable(){
	assert (!interruptsAreDisabled);
	interruptsAreDisabled = 1;
}

static void interruptEnable(){
	assert(interruptsAreDisabled);
	interruptsAreDisabled = 0;
}

//recursive free function (everytime calling this function will free
//any threads that are done)
void freeStack(int a){
	ucontext_t temp=*(ptrC[a]->th);
	free(temp.uc_stack.ss_sp);
	free(ptrC[a]->th);
}

//wrapper function to ease implementation
void wrapperFunc(thFuncPtr funcPtr, void* argPtr, int id){

	//start function execution
	if(interruptsAreDisabled)
	  interruptEnable();

	ptrC[id]->resultH = funcPtr(argPtr);

	if(!interruptsAreDisabled)
	 interruptDisable();

	//function returned, mark the thread as Done
	ptrC[id]->status=DONE;

	//decrement total thread number
	totalThread--;

	//free the finished thread
	freeStack(id);

	int i;

	//finding next thread available
	for(i=0;i<lastIndex;i++){
		if(ptrC[i]->status==WAIT)
			break;
	}	
		
	ptrC[i]->status=CURRENT;

  	setcontext(ptrC[i]->th);
}

void threadInit(){

  if(!interruptsAreDisabled)
  interruptDisable();

  //Delacring and mallocing new arrays
  ptrC=(cs**)malloc(START_SIZE*sizeof(cs*));

  status_lock=(int*)malloc(NUM_LOCKS*sizeof(int));
  conditions_lock=(int**)malloc(NUM_LOCKS*sizeof(int*));

  //initializing
  int a=0;
  for(;a<NUM_LOCKS;a++){
  	conditions_lock[a]=calloc(CONDITIONS_PER_LOCK,sizeof(int));
  }


  for(a=0;a<START_SIZE;a++){
    ptrC[a]=NULL;
  }

  for(a=0;a<NUM_LOCKS;a++){
  	status_lock[a]=0;
  }

  //settup the first thread which is the main thread
  ptrC[0]=(cs*)malloc(sizeof(cs));
  ptrC[0]->th=malloc(sizeof(ucontext_t));
  ptrC[0]->status=CURRENT;
  ptrC[0]->resultH=NULL;

  if(interruptsAreDisabled)
  interruptEnable();
}



int threadCreate(thFuncPtr funcPtr, void *argPtr){

	if(!interruptsAreDisabled)
	interruptDisable();
	
	//making new thread according to the current thread
    ptrC[lastIndex]=(cs*)malloc(sizeof(cs));
    ptrC[lastIndex]->th=malloc(sizeof(ucontext_t));
   	ptrC[lastIndex]->status=WAIT;
   	ptrC[lastIndex]->resultH=NULL;

    getcontext (ptrC[lastIndex]->th);

	(*(ptrC[lastIndex]->th)).uc_stack.ss_sp = malloc(STACK_SIZE);
	(*(ptrC[lastIndex]->th)).uc_stack.ss_size = STACK_SIZE;
	(*(ptrC[lastIndex]->th)).uc_stack.ss_flags = 0;	

	makecontext(ptrC[lastIndex]->th,
				(void(*)(void))wrapperFunc,3,funcPtr,argPtr,lastIndex);

	totalThread++;
    lastIndex++;

    //resize if the array is not large enough
  	if(lastIndex==curSize){
    	curSize*=2;
    	ptrC=realloc(ptrC,curSize*sizeof(cs*));
  	}

  	//record the current index as the thread id
  	int tempIndx=lastIndex-1;

  	//looking for the current thread which called this threadCreate
  	int a=0;
  	for(;a<lastIndex;a++){
  		if(ptrC[a]->status==CURRENT)
  			break;
  	}

  	//mark their status
  	ptrC[a]->status=WAIT;
	ptrC[tempIndx]->status=CURRENT;

	//start the new context execution
  	swapcontext(ptrC[a]->th,ptrC[tempIndx]->th);

  	if(interruptsAreDisabled)
  		interruptEnable();

  	//return the thread id
	return tempIndx;
}


void threadYield(){
	if (!interruptsAreDisabled)
	interruptDisable();

	//if there is only main thread then return immediately
	if(totalThread==1){

		if (interruptsAreDisabled)
		interruptEnable();

		return;
	}

	int i;
	int a;

	//looking for the current thread
	for(a=0;a<lastIndex;a++){
		if(ptrC[a]->status==CURRENT){
			break;
		}
	}

	//finding the next available thread
	for(i=a;i<lastIndex;i++){
		if(ptrC[i]->status==WAIT)
			break;
	}

	//rollover to 0 if nothing was found
	if(i==lastIndex){
		for(i=0;i<a;i++){
			if(ptrC[i]->status==WAIT)
				break;
		}
	}

	//if none of the threads are available, then return
	if(i==a){

		if (interruptsAreDisabled)
		interruptEnable();

		return;
	}
	else{

	//otherwise, mark the state, and swap
		ptrC[a]->status=WAIT;
		ptrC[i]->status=CURRENT;

	}

	swapcontext(ptrC[a]->th,ptrC[i]->th);

	if (interruptsAreDisabled)
	interruptEnable();
}

void threadJoin(int thread_id, void **result){

	if(!interruptsAreDisabled)
	 interruptDisable();

	//if the thread does not exist or has finished
	//check the results array and record it in the result variable
	//return it back to the caller
	if(ptrC[thread_id]->status==NOT_EXIT){
		if(interruptsAreDisabled)
	 	interruptEnable();
		return;
	}

	if(ptrC[thread_id]->status==DONE){

		if(ptrC[thread_id]->resultH!=NULL){
			*result=ptrC[thread_id]->resultH;
		}

		if(interruptsAreDisabled)
	 	interruptEnable();
		return;
	}

	//otherwise yield processor to other threads
	//wait it to finish
	while(ptrC[thread_id]->status!=DONE){

		threadYield();

		if(!interruptsAreDisabled)
		 interruptDisable();
	}

	//record result
	if(ptrC[thread_id]->resultH!=NULL){
		*result=ptrC[thread_id]->resultH;
	}

	if (interruptsAreDisabled)
	interruptEnable();
}

void threadExit(void *result){
	if(!interruptsAreDisabled)
	interruptDisable();

	//find the thread that is calling exit
	int a=0;
	for(;a<lastIndex;a++){
		if(ptrC[a]->status==CURRENT)
			break;
	}

	//if not main thread
	if(a!=0){

		//record result, mark status
		ptrC[a]->resultH=result;
		ptrC[a]->status=DONE;
		totalThread--;
		freeStack(a);

		int i;

		//finding next thread available
		for(i=0;i<lastIndex;i++){
			if(ptrC[i]->status==WAIT)
				break;
		}	
		
		ptrC[i]->status=CURRENT;

	  	setcontext(ptrC[i]->th);
	}
	//if it is main thread
	else{
	for(a=0;a<lastIndex;a++){

		//free everything and then exit
			if(ptrC[a]!=NULL&&ptrC[a]->status!=DONE){
				ucontext_t temp;
				temp=*(ptrC[a]->th);
				free(temp.uc_stack.ss_sp);
	
				free(ptrC[a]->th);
				free(ptrC[a]);
	
			}
		}
		free(ptrC);
		exit(0);
	}

}


void threadLock(int lockNum){
	if(!interruptsAreDisabled)
	interruptDisable();

	//waiting for lock
	while(status_lock[lockNum]!=0){

		threadYield();
		
		if(!interruptsAreDisabled)
		interruptDisable();
	}

	//lock it after acquired
	status_lock[lockNum]=1;

	if(interruptsAreDisabled)
	interruptEnable();
}

void threadUnlock(int lockNum){
	if(!interruptsAreDisabled)
	interruptDisable();

	//unlock it
	status_lock[lockNum]=0;

	if(interruptsAreDisabled)
	interruptEnable();
}

void threadWait(int lockNum, int conditionNum){
	if(!interruptsAreDisabled)
	interruptDisable();

	//check if the lock is already locked
	if(status_lock[lockNum]!=1){
		fprintf(stderr, "lock %d is not locked\n", lockNum);
		
		exit(0);
	}

	//unlock
	status_lock[lockNum]=0;

	//set condition variable flag
	conditions_lock[lockNum][conditionNum]=1;

	//wait for signal
	while(conditions_lock[lockNum][conditionNum]==1){
		threadYield();
	
		if(!interruptsAreDisabled)
		interruptDisable();
	}
	
	//lock again
	status_lock[lockNum]=1;

	if(interruptsAreDisabled)
	interruptEnable();
} 

void threadSignal(int lockNum, int conditionNum){
	if(!interruptsAreDisabled)
	interruptDisable();

	//free the condition variable
	conditions_lock[lockNum][conditionNum]=0;


	if(interruptsAreDisabled)
	interruptEnable();
}