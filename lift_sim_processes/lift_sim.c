/***************************************************************************
* AUTHOR: Kade McGarraghy                                                  *
* LAST MODIFIED: 10/5/20                                                   *
* FILE NAME: lift_sim.c                                                    *
* PURPOSE: To simulate a lift system with 3 lifts and 50-100 lift requests *
***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include "lift_sim.h"
#include "file_reader.h"

#define NUM_PROCESSES 4
#define MAX_REQUESTS 100
#define MEM_SIZE 4096

int n = 0; /* number of requests */
int m = 1; /* buffer size */
int t = 0; /* lift duration time */
Request* buffer;
int* in; /* points to next free position in buffer*/
int* out; /* points to first full position in buffer*/
int* requestsLeft;

sem_t* mut;
sem_t* full;
sem_t* empty;
sem_t* wrt;
sem_t* removeRequest;

Request requests[MAX_REQUESTS];

typedef struct
{
	int liftNum;
	int requestsServed;
	int totalMovement;
} LiftData;

LiftData* args;

typedef struct
{
	int counter;
	int in;
	int out;
	int requestsLeft;
	int buffer;
	int mut;
	int full;
	int empty;
	int wrt;
	int removeRequest;
	int args;
}
Fds;

void* request(void* arg);
void* lift(void* arg);
void create_shared_memory(Fds fds);
void destroy_shared_memory();

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
			
			pid_t pid; 
			pid_t parentPid = getpid();
			int liftNum;
			Fds fds;
			FILE* outputFile;
			
			create_shared_memory(fds);
			
			*in = 0;
			*out = 0;
			*requestsLeft = n;
			
			/* creates and initialises semaphores */
			sem_init(mut, 1, 1);
			sem_init(empty, 1, m);
			sem_init(full, 1, 0);
			sem_init(wrt, 1, 1);
			sem_init(removeRequest, 1, 1);
			
			printf("\nRunning simulation...\n");
			
			outputFile = fopen("sim_out.txt", "w"); /*make sure file is clean*/
			fclose(outputFile);
					
			for (i = 0; i < (NUM_PROCESSES - 1); i++)
			{
				if (getpid() == parentPid) /* makes sure that only the parent process forks. */
				{
					pid = fork();
					liftNum = i;
				}
			}
			
			if (pid < 0)
			{
				exit(EXIT_FAILURE);
			}
			else if (pid == 0) /* child processes*/
			{
				args[liftNum].liftNum = (liftNum + 1);
				lift(&args[liftNum]);
			}
			else if (pid > 0) /* parent process*/
			{
				request(NULL);
				
				int status;
				for (i = 0; i < (NUM_PROCESSES - 1); i++)
				{
					wait(&status);
				}
				
				movementTotal = args[0].totalMovement + args[1].totalMovement + args[2].totalMovement; 
				requestTotal = args[0].requestsServed + args[1].requestsServed + args[2].requestsServed;
				
				outputFile = fopen("sim_out.txt", "a");
				fprintf(outputFile, "\nTotal number of requests: %d"
								   "\nTotal number of movements: %d",
								   requestTotal, movementTotal);
				fclose(outputFile);
				
				sem_destroy(mut);
				sem_destroy(empty);
				sem_destroy(full);
				sem_destroy(wrt);
				
				destroy_shared_memory();
				
				printf("\nSimulation complete. View results in 'sim_out.txt'\n");
			}
		}
	}

	return 0;
}

void* request(void* arg) /* = PRODUCER PROCESS USING BOUNDED BUFFER*/
{
	FILE* outputFile;
	
	Request nextRequest; /* item next produced;*/
	int requestNo = 0;
	int done = FALSE;
	
	while (done == FALSE) 
	{

		nextRequest = requests[requestNo]; /* Produce an item in nextProduced*/
		requestNo++; 
		
		/* start critical section */
		sem_wait(empty);
		sem_wait(mut);
		
		buffer[*in] = nextRequest;
		*in = (*in + 1) % m;
					
		/* end critical section */
		sem_post(mut);
		sem_post(full);
							
		sem_wait(wrt);
		outputFile = fopen("sim_out.txt", "a");
		fprintf(outputFile, "--------------------------------------------\n"
			"New Lift Request From Floor %d to Floor %d\n"
			"Request No: %d\n"
			"--------------------------------------------\n\n",
			nextRequest.from, nextRequest.to, requestNo);
		fclose(outputFile);
		sem_post(wrt);
		
		sleep(0);
		
		if (requestNo == n)
		{
			done = TRUE;
		}
	}
    
    return NULL;
}

void* lift(void* arg) /* = CONSUMER PROCESS USING BOUNDED BUFFER*/
{
	LiftData* argStruct = (LiftData*) arg;
	FILE* outputFile;
	Request nextHandled; /* item next consumed;*/
	int firstMovement;
	int secondMovement;
	int combinedMovement;
	int prevPosition = 1;
	int position = 1;
	int requestFrom = 1;
	int requestTo = 1;
	int requestsServed = 0;
	int totalMovement = 0;

	while (*requestsLeft > 0) 
	{
		sem_wait(removeRequest);
		*requestsLeft = *requestsLeft - 1;
		sem_post(removeRequest);
		
		/* start critical section */
		sem_wait(full);
		sem_wait(mut);
		
		nextHandled = buffer[*out];
		*out = (*out + 1) % m;
		
		/* end critical section */
		sem_post(mut);
		sem_post(empty);
			
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
		
		sem_wait(wrt);
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
		sem_post(wrt);
		
		sleep(t);
	
	}
	argStruct->totalMovement = totalMovement;
	argStruct->requestsServed = requestsServed;
	
    return NULL;
}

void create_shared_memory(Fds fds)
{
	/*create shared memory objects*/
	fds.in = shm_open("in_sm", O_CREAT | O_RDWR, 0666); 
	fds.out = shm_open("out_sm", O_CREAT | O_RDWR, 0666);
	fds.requestsLeft = shm_open("requestsLeft_sm", O_CREAT | O_RDWR, 0666); 
	fds.buffer = shm_open("buffer_sm", O_CREAT | O_RDWR, 0666); 
	fds.mut = shm_open("mut_sm", O_CREAT | O_RDWR, 0666);
	fds.full = shm_open("full_sm", O_CREAT | O_RDWR, 0666);
	fds.empty = shm_open("empty_sm", O_CREAT | O_RDWR, 0666);
	fds.wrt = shm_open("wrt_sm", O_CREAT | O_RDWR, 0666);
	fds.removeRequest = shm_open("removeRequest_sm", O_CREAT | O_RDWR, 0666);
	fds.args = shm_open("args_sm", O_CREAT | O_RDWR, 0666);

	/*configure sizes of shared memory objects*/
	ftruncate(fds.in, sizeof(int));
	ftruncate(fds.out, sizeof(int));
	ftruncate(fds.requestsLeft, sizeof(int));
	ftruncate(fds.buffer, sizeof(Request) * m);
	ftruncate(fds.mut, sizeof(sem_t));
	ftruncate(fds.full, sizeof(sem_t));
	ftruncate(fds.empty, sizeof(sem_t));
	ftruncate(fds.wrt, sizeof(sem_t));
	ftruncate(fds.removeRequest, sizeof(sem_t));
	ftruncate(fds.args, sizeof(LiftData) * (NUM_PROCESSES - 1));
	
	/*mem map shared memory objects to pointers*/
	in = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, fds.in, 0);
	out = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, fds.out, 0);
	requestsLeft = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, fds.requestsLeft, 0);
	buffer = (Request*)mmap(NULL, sizeof(Request) * m, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, fds.buffer, 0);
	mut = (sem_t*)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, fds.mut, 0);
	full = (sem_t*)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, fds.full, 0);
	empty = (sem_t*)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, fds.empty, 0);
	wrt = (sem_t*)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, fds.wrt, 0);
	removeRequest = (sem_t*)mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, fds.removeRequest, 0);
	args = (LiftData*)mmap(NULL, sizeof(LiftData) * (NUM_PROCESSES - 1), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, fds.args, 0);
}

void destroy_shared_memory()
{
	munmap(in, sizeof(int));
	munmap(out, sizeof(int));
	munmap(requestsLeft, sizeof(int));
	munmap(buffer, sizeof(Request) * m);
	munmap(mut, sizeof(sem_t));
	munmap(empty, sizeof(sem_t));
	munmap(full, sizeof(sem_t));
	munmap(wrt, sizeof(sem_t));
	munmap(removeRequest, sizeof(sem_t));
	munmap(args, sizeof(LiftData));
	
	shm_unlink("in_sm");
	shm_unlink("out_sm");
	shm_unlink("requestsLeft_sm");
	shm_unlink("buffer_sm");
	shm_unlink("mut_sm");
	shm_unlink("full_sm");
	shm_unlink("empty_sm");
	shm_unlink("wrt_sm");
	shm_unlink("removeRequest_sm");
	shm_unlink("args_sm");
}
	
	
			
