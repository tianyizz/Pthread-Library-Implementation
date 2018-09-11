//threadtest.c

#include <stdio.h>
#include <stdlib.h>
#include "mythreads.h"




void *t1 (void *arg)
{
	//threadLock(1);

	int param = *((int*)arg);
	printf("t1 started %d\n",param);

	threadYield();

	int* result = malloc(sizeof(int));
	*result = param + 1;
	printf ("added 1! (%d)\n",*result);
	
	threadYield();
	printf("t1: done result=%d\n",*result);

		//	threadUnlock(1);
/*

	int *temp=malloc(sizeof(int));
	*temp=99;
	threadExit((void*)temp);
*/
	return result;
}


void *t2 (void* ptr){

	//threadExit(NULL);

	//threadLock(1);


	printf("t2 started %d\n",*(int*)ptr);
/*
		int p1;

		p1=1001;

		int id1;

	id1 = threadCreate(t2,&p1);
		printf("created thread 3 %d.\n",id1);	
*/

	threadYield();

//int *result1;

	//		threadJoin(id1, (void*)&result1);
	//printf("joined #3 -->%d.\n",*(result1));


	int* result = malloc(sizeof(int));
	*result = *(int*)ptr + 1;
	printf ("added 1! (%d)\n",*result);
	
	threadYield();

	printf("t1: done result=%d\n",*result);

	int *temp=malloc(sizeof(int));
	*temp=99;
	threadExit((void*)temp);


	return result;
}


int main(void)
{

	int id1, id2;
	
	
	int p1;
	int p2;

	p1 = 23;
	p2 = 2;

	int *result1, *result2;

	//initialize the threading library. DON'T call this more than once!!!
	threadInit();

	id1 = threadCreate(t2,&p1);
	printf("created thread 1.\n");	

	id2 = threadCreate(t2,&p2);
	printf("created thread 2.\n");

	threadJoin(id1, (void*)&result1);
	printf("joined #1 -->%d.\n",*(result1));

	threadJoin(id2, (void*)&result2);
	printf("joined #2 -->%d.\n",*(result2));


}

