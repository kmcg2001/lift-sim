/***************************************************************************
* AUTHOR: Kade McGarraghy                                                  *
* LAST MODIFIED: 10/5/20                                                   *
* FILE NAME: lift_sim.c                                                    *
* PURPOSE: To simulate a lift system with 3 lifts and 50-100 lift requests *
***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "lift_sim.h"
#include "file_reader.h"

#define NUM_THREADS 4
#define MAX_REQUESTS 100

int n = 0; /* number of requests */
int m = 1; /* buffer size */
int t = 0; /* lift duration time */
Request* buffer;
int in = 0; /* points to next free position in buffer*/
int out = 0; /* points to first full position in buffer*/
int requestsLeft = 0;
int counter = 0;

Request requests[MAX_REQUESTS];

typedef struct
{
	int liftNum;
	int requestsServed;
	int totalMovement;
} LiftData;

pthread_mutex_t mut; 
pthread_mutex_t wrt;
pthread_mutex_t removeRequest;
pthread_cond_t empty;  
pthread_cond_t full;

void* request(void* arg);
void* lift(void* arg);

int main(int argc, char* argv[])
{	
    if (argc != 3)
    {
        printf("\nUsage: ./lift_sim_A <m> <t>\nwhere m = buffer size and t = time each lift takes to serve a request\n");
    }
	else
	{
		int i;
		int success;
		int requestTotal = 0;
		int movementTotal = 0;
		
		success = readInputFile(requests, &n);
		m = atoi(argv[1]);
		t = atoi(argv[2]);
		if ((m < 1) || (t < 0))
		{
			printf("\nError: m must be 1 or greater and t must be 0 or greater\n");
			success = FALSE;
		}

		if (success == TRUE)
		{
			FILE* outputFile;
			
			outputFile = fopen("sim_out.txt", "w"); /*make sure file is clean*/
			fclose(outputFile);
			
			
			LiftData args[NUM_THREADS - 1];
			int liftNums[NUM_THREADS - 1];
			pthread_t lifts[NUM_THREADS - 1]; /* creates lift (thread) IDs*/
			pthread_t liftR;
			pthread_attr_t attr; /* Create attributes */
			
			pthread_mutex_init(&mut, NULL);
			pthread_mutex_init(&wrt, NULL);
			pthread_mutex_init(&removeRequest, NULL);
			pthread_cond_init(&empty, NULL);
			pthread_cond_init(&full, NULL);
			
			buffer = (Request*)malloc(m * sizeof(Request));
			requestsLeft = n;
				
			printf("\nRunning simulation...\n");
			
			pthread_attr_init(&attr); /* sets attributes to default*/
			pthread_create(&liftR, &attr, request, NULL);
			
            for (i = 0; i < (NUM_THREADS - 1); i++)
			{		
				pthread_attr_t attr; /* Create attributes:*/
				pthread_attr_init(&attr); /* sets attributes to default*/
				liftNums[i] = i;
				args[i].liftNum = (liftNums[i] + 1);
				pthread_create(&lifts[i], &attr, lift, &args[i]);
			}
			
			pthread_join(liftR, NULL); /* Waits until all threads have done their work before finishing main*/
			
			for (i = 0; i < (NUM_THREADS - 1); i++)
			{
				pthread_join(lifts[i], NULL);
			}
			
			movementTotal = args[0].totalMovement + args[1].totalMovement + args[2].totalMovement;
			requestTotal = args[0].requestsServed + args[1].requestsServed + args[2].requestsServed;
            
            outputFile = fopen("sim_out.txt", "a");
			fprintf(outputFile, "\nTotal number of requests: %d"
							   "\nTotal number of movements: %d",
							   requestTotal, movementTotal);
            fclose(outputFile);
			
			pthread_mutex_destroy(&mut);
			pthread_mutex_destroy(&wrt);
			pthread_mutex_destroy(&removeRequest);
			pthread_cond_destroy(&full);
			pthread_cond_destroy(&empty);
			
			free(buffer);
			
			printf("\nSimulation complete. View results in 'sim_out.txt'\n");
		}
	}
	
	return 0;
}

void* request(void* arg) /* = PRODUCER PROCESS USING BOUNDED BUFFER*/
{
	FILE* outputFile;
	Request nextRequest; /* item nextProduced;*/
	int requestNo = 0;
	int done = FALSE;
	
	while (done == FALSE) 
	{
	
		nextRequest = requests[requestNo]; /* Produce an item in nextProduced*/
		requestNo++;

		/* start critical section */
		pthread_mutex_lock(&mut);
		
	
		while (counter == m) 
		{
			pthread_cond_wait(&empty, &mut); /* releases lock, waits for signal, then waits to re-acquire lock */
		}
		
		buffer[in] = nextRequest;
		in = (in + 1) % m;
		counter = counter + 1;
		
		/* end critical section */
		pthread_cond_signal(&full); /*sends signal to thread waiting for condition variable*/
		pthread_mutex_unlock(&mut);

		pthread_mutex_lock(&wrt);
		outputFile = fopen("sim_out.txt", "a");
		fprintf(outputFile, "--------------------------------------------\n"
							"New Lift Request From Floor %d to Floor %d\n"
							"Request No: %d\n"
							"--------------------------------------------\n\n",
							nextRequest.from, nextRequest.to, requestNo);
		fclose(outputFile);
		pthread_mutex_unlock(&wrt);
		sleep(0);
		
		if (requestNo == n) 
		{
			done = TRUE;
		}
		
		
	}
	pthread_exit(0);
}

void* lift(void* arg) /* = CONSUMER PROCESS USING BOUNDED BUFFER*/
{
	LiftData* argStruct = (LiftData*) arg;
	
	FILE* outputFile;
	Request nextHandled; /* item nextConsumed;*/
	int firstMovement;
	int secondMovement;
	int combinedMovement;
	int prevPosition = 1;
	int position = 1;
	int requestFrom = 1;
	int requestTo = 1;
	int done = FALSE;
	int requestsServed = 0;
	int totalMovement = 0;

	while (done == FALSE) 
	{
		pthread_mutex_lock(&removeRequest);
		requestsLeft = requestsLeft - 1;
		pthread_mutex_unlock(&removeRequest);
		
		/* start critical section */
		pthread_mutex_lock(&mut);
		
		while (counter == 0)
		{
			pthread_cond_wait(&full, &mut);
		}
		
		nextHandled = buffer[out];
		out = (out + 1) % m;
		counter = counter - 1;
		
		/* end critical section */
		pthread_cond_signal(&empty); 
		pthread_mutex_unlock(&mut);
		
		
		requestFrom = nextHandled.from; /* Consume the next item in nextConsumed*/
		requestTo = nextHandled.to;
		requestsServed++; 
		
		prevPosition = position; 
		
		/* go from position of lift to floor request came from */
		if (requestFrom != prevPosition)
		{
			position = requestFrom;
		}
		firstMovement = abs(position - prevPosition);
		
		/* go from floor request came from to floor request is to.*/
		if (position != requestTo)
		{
			position = requestTo;
		}
		secondMovement = abs(position - requestFrom);
		
		combinedMovement = firstMovement + secondMovement;
		totalMovement = totalMovement + combinedMovement; 
		
		pthread_mutex_lock(&wrt);
		outputFile = fopen("sim_out.txt", "a");
		fprintf(outputFile, "Lift-%d Operation\n"
							"Previous position: Floor %d\n"
							"Request: Floor %d to Floor %d\n"
							"Detail operations:\n"
								"\tGo from Floor %d to Floor %d\n"
								"\tGo From Floor %d to Floor %d\n"
								"\t#movement for this request: %d\n"
								"\t#request: %d\n" 
								"\tTotal #movement: %d\n"
							"Current position: %d\n\n",
                            argStruct->liftNum, prevPosition, requestFrom, requestTo, prevPosition, requestFrom, requestFrom, requestTo, combinedMovement, requestsServed,
                            totalMovement, position);
		fclose(outputFile);
		pthread_mutex_unlock(&wrt);
		sleep(t);

		if (requestsLeft == 0)
		{
			done = TRUE;
		}
	}
	argStruct->totalMovement = totalMovement;
	argStruct->requestsServed = requestsServed;
	
	pthread_exit(0);
}
