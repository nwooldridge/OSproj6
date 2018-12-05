#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <sys/msg.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <math.h>

#define true 1
#define false 0

//Queue structure
//Source: https://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation/
struct Queue 
{ 
    int front, rear, size; 
    unsigned capacity; 
    int* array; 
}; 
typedef struct Queue Queue;	

//initialize queue
struct Queue* createQueue(unsigned capacity) 
{ 
    struct Queue* queue = (struct Queue*) malloc(sizeof(struct Queue)); 
    queue->capacity = capacity; 
    queue->front = queue->size = 0;  
    queue->rear = capacity - 1;  // This is important, see the enqueue 
    queue->array = (int*) malloc(queue->capacity * sizeof(int)); 
    return queue; 
} 

//return bool value if full or not
int isFull(struct Queue* queue) {
	return (queue->size == queue->capacity); 
} 

//return bool value if empty or not
int isEmpty(struct Queue* queue) {
return (queue->size == 0);
}

//enqueue item at rear of queue
void enqueue(struct Queue* queue, int item) 
{ 
    if (isFull(queue)) 
        return; 
    queue->rear = (queue->rear + 1)%queue->capacity; 
    queue->array[queue->rear] = item; 
    queue->size = queue->size + 1; 
    //printf("%d enqueued to queue\n", item); 
} 

//dequeue item from front of queue
int dequeue(struct Queue* queue) 
{ 
    if (isEmpty(queue)) 
        return INT_MIN; 
    int item = queue->array[queue->front]; 
    queue->front = (queue->front + 1)%queue->capacity; 
    queue->size = queue->size - 1; 
    return item; 
}
 
//return front of queue
int front(struct Queue* queue) 
{ 
    if (isEmpty(queue)) 
        return INT_MIN; 
    return queue->array[queue->front]; 
}
 
//return rear of queue
int rear(struct Queue* queue) 
{ 
    if (isEmpty(queue)) 
        return INT_MIN; 
    return queue->array[queue->rear]; 
}  

//struct for keeping track of child processes
struct process {
	//process id
	pid_t processID;
	//array to represent virtual address space
	int pageTable[32];
};
typedef struct process process;

struct frame {
	//Process id
	pid_t processID;
	//reference bit for second-chance FIFO page replacement algorithm
	int referenceBit;
	//dirty bit
	int dirtyBit;
	//0 for not allocated, 1 for allocated
	int allocated;
	
};
typedef struct frame frame;

//message queue structure
struct mesgBuffer {
        long mesgType;
	//Process id
        pid_t pid;
	//Number from 0 - 32
	int pageNumber;
	//0 for read, 1 for write
	int readOrWrite;
} message;

process * PCB;
int shmid, msgid;

void handle_sigint(int sig) {

        printf("\nCaught signal: %d\n", sig);
        if (shmctl(shmid, IPC_RMID, NULL) < 0)
                perror("shmctl oss: ");
        if (msgctl(msgid, IPC_RMID, NULL) < 0)
                perror("msgctl oss: ");
        if (shmdt(PCB) < 0)
                perror("shmdt oss: ");
	exit(-1);
}



//size of PCB
const int MAX_PROCESSES = 18;

int main(int argc, char ** argv) {
	//loop counters
	int i, j, k;

	FILE * f = fopen("log.txt", "a");
	
	int writes = 0;

	//simulated clock
	unsigned int sClock[2];	
	sClock[0] = 0; sClock[1] = 0;
	
	//keeps track of amount of active processes
	int processCounter = 0;
	//keeps track of total page faults 
	int pageFaults = 0;
	int memoryAccesses = 0;
	
	//queue for keeping track of memory references
	Queue * memoryReferenceQueue = createQueue(256);

	//array for representing physical memory
	frame frames[256];
	
	//signal handling
	signal(SIGINT, handle_sigint);	
	
	//seeding random
	srand(time(NULL));
	
	//Create key for msgqueue	
	key_t msgKey = ftok("msgqfile", 51);
	
	//Create key for shared memory
	key_t shmKey = ftok("shmfile", 12);

	//error checking for ftok
	if (msgKey == -1 || shmKey == -1)
		perror("ftok: ");
	
	//create message queue
	msgid = msgget(msgKey, 0666|IPC_CREAT);
	
	//create shared memory segment for process control blocks
	shmid = shmget(shmKey, MAX_PROCESSES*sizeof(process), IPC_CREAT | 0666);
	
	//error checking for msgget and shmget	
	if (msgid == -1)
		perror("msgget oss: ");
	
	if (shmid == -1)
		perror("shmget oss: ");
	
	//attach shared memory segment to process control block pointer
	PCB = (process * )shmat(shmid, (void *)0, 0);
	
	//initialize PCB's
	for (i = 0; i != MAX_PROCESSES; i += 1) {
		
		//set all process id's to 0	
		PCB[i].processID = 0;

		//set page tables to contain all -1
		for (j = 0; j != 32; j += 1)
			PCB[i].pageTable[j] = -1;

	}
	
	//initialize frame table
	for (i = 0; i != 256; i += 1) {
		frames[i].referenceBit = 0;
		frames[i].dirtyBit = 0;
		frames[i].allocated = 0;
	}
	
	
	/*
	if (msgsnd(msgid, &message, sizeof(message), 0))
		perror("msgsnd oss");
	*/

	//terminates loop when set to 0
	int breakLoop = 1;
	//keep track of time to next fork
	unsigned int nextFork[2];
	nextFork[0] = 0; nextFork[1] = 0;
	//time at start of program
	time_t startTime = time(NULL);
	
	//main loop, set breakLoop to 0 to stop loop
	while (breakLoop) {
		
		//break out of loop after 10 real time seconds
		if (difftime(time(NULL), startTime) >= 10) {
			breakLoop = 0;
		}
		
		//if enough time has passed to fork
		if ((sClock[0] >= nextFork[0]) && (sClock[1] >= nextFork[1]) && (processCounter < 18)) {

			processCounter += 1;
			pid_t childpid = fork();

			//fork error if returns -1
			if (childpid == -1)
				perror("fork: ");
			
			//in child process
			else if (childpid == 0){
				
				for (i = 0; i != MAX_PROCESSES; i += 1){
                        		
					//creating process control block
                        		if (PCB[i].processID == 0) {
					
                                		PCB[i].processID = getpid();
						if (writes < 10000) {
                                			fprintf(f, "creating process with pid %d and assigning it to P%d\n", getpid(), i);
							writes += 1;
						}
	
						break;
                        		}
                		}		
				
				//executes user process
				execl("./user", "user", NULL);
				//if it makes it this far, its an error
                		perror("execlp: ");
	
			}
			
			//increase time to next fork by random value between 10 and 5000 milliseconds
			nextFork[1] = pow(10, 7) * (rand() % 500 + 1);
	
			//for every billion nanoseconds add 1 second and subtract billion nanoseconds
			while (nextFork[1] > 999999999) {
				nextFork[1] -= pow(10, 9);
				nextFork[0] += 1;
			}	
			while (sClock[1] > 999999999) {
				sClock[1] -= pow(10, 9);
				sClock[0] += 1;
			}
	
		}
	
		//receive message from user processes
		if (msgrcv(msgid, &message, sizeof(message), 1, 0) < 0)
               		perror("msgrcv: ");

		memoryAccesses += 1;

		for (i = 0; i != MAX_PROCESSES; i += 1) {
			if (PCB[i].processID == message.pid)
				break;
		}
		int currentProcess = i;
		
		//if message is read or write request
		if (message.readOrWrite == 0 || message.readOrWrite == 1) { 
                	
			if (message.readOrWrite == 0)
				if (writes < 10000) {
					fprintf(f, "Process %d requesting read to page %d at time %d : %d\n", message.pid, message.pageNumber, sClock[0], sClock[1]);
					writes += 1;
				}
                	else if (message.readOrWrite == 1)
				if (writes < 10000) {
					fprintf(f, "Process %d requesting write to page %d at time %d : %d\n", message.pid, message.pageNumber, sClock[0], sClock[1]);
					writes += 1;
				}
                	else
				printf("error with something\n");
			
			//increment clock by 15 milliseconds
			//decrement clock by 1 billion nanoseconds and add 1 to seconds
			sClock[1] += 15 * pow(10, 6);
			if (sClock[1] > 999999999) {
				sClock[1] -= pow(10, 9);
				sClock[0] += 1;
			}
			
			//stores temporary values from queue
			int tmp;
	
			
			//if page requested isnt in a frame
			if (PCB[i].pageTable[message.pageNumber] == -1) {


				if (writes < 10000) {
                        		fprintf(f, "page %d not in frame, page fault\n", message.pageNumber);
					writes += 1;
				}
				pageFaults += 1;

				//if queue is full then oss performs swap of pages
				if (isFull(memoryReferenceQueue)) {
					
					//iterate through queue finding frame to write new page to
					for (i = 0; 1; i += 1) {
						
						//dequeue item and store into tmp variable
						tmp = dequeue(memoryReferenceQueue);

						//dequeue returns min_int if queue is empty
						if (tmp < 0)
							printf("issues dequeueing, queue empty\n");
						//otherwise a frame is returned
						else {
							
							//checks reference bit as part of second chance
							//if it is 0, then this frame can be used
							if (frames[tmp].referenceBit == 0) {
								
								//deallocate previous page pointing to tmp frame
								for (j = 0; j != MAX_PROCESSES; j += 1) {
														
									//if pcb with matching pid is found
									if (PCB[j].processID == frames[tmp].processID) {
											
										//iterate through pages to find frame tmp
										for (k = 0; k != 32; k += 1) {
		
											//found matching frame number
											if (PCB[j].pageTable[k] == tmp) {
												PCB[j].pageTable[k] = -1;
												break;
											}
										}
										//if we iterate through entire loop, that means bigger issues
										if (k == 32)
											printf("error finding page pointing to tmp frame\n");
										
										break;
									}

								}
								//no matching pid is found in PCB.
								if (j == MAX_PROCESSES)
									printf("didn't find process with specified pid in PCB's\n");
								if (writes < 10000) {
									fprintf(f, "swapping out page %d with page %d into frame %d\n", k, message.pageNumber, tmp);					
									writes += 1;
								}
			
								//set new page to reference frame tmp
								PCB[currentProcess].pageTable[message.pageNumber] = tmp;
								
								//set process id on frame to new process
								frames[tmp].processID = message.pid;

								//set dirty bit if it is a write
								if (message.readOrWrite == 1)
									frames[tmp].dirtyBit = 1;
								else
									frames[tmp].dirtyBit = 0;

								//set reference bit to 0
								frames[tmp].referenceBit = 0;

								//put back into queue
								enqueue(memoryReferenceQueue, tmp);
								break;
							
							}
								
							//otherwise reference bit is 1 so we move on in queue
							else {
								
								//set reference bit back to 0
								frames[tmp].referenceBit = 0;

								//put back into queue to move to next item
								enqueue(memoryReferenceQueue, tmp);

							}
						}
		
					}
					
					//printf("queue is full, performing swap...\n");
				}
				
				//if queue is not full
				else {

					//randomly find a frame number to start at
					int startFrame = rand() % 256;
					for (i = 0; i != 256; i += 1) {
				
						//avoid seg fault
						if (startFrame > 255)
							startFrame -= 256;
				
						//finding unallocated frame
						if (frames[startFrame].allocated == 0) {
	
							//putting page into frame and initializing frame and page
							PCB[currentProcess].pageTable[message.pageNumber] = startFrame;
							frames[startFrame].allocated = 1;
							if (message.readOrWrite == 1)
								frames[startFrame].dirtyBit = 1;
							else
								frames[startFrame].dirtyBit = 0;
							frames[startFrame].referenceBit = 0;
							frames[startFrame].processID = message.pid;
							enqueue(memoryReferenceQueue, startFrame);
						
							if (writes < 10000) {
								fprintf(f, "putting page %d into frame %d\n", message.pageNumber, startFrame);		
								writes += 1;
							}
							break;
							
						}
						startFrame += 1;
					}
					if (i == 256)
						printf("issue with finding unallocated frame\n");
			
				
				}
                	}
			//otherwise the page is already in a frame
			else {
				if (writes < 10000) {
					fprintf(f, "page %d already in frame %d\n", message.pageNumber, PCB[currentProcess].pageTable[message.pageNumber]);
					writes += 1;
				}

				if (message.readOrWrite == 1) { //write
					//set dirty bit and increment clock
					frames[PCB[currentProcess].pageTable[message.pageNumber]].dirtyBit = 1;
					sClock[1] += 10;
					frames[PCB[currentProcess].pageTable[message.pageNumber]].referenceBit = 1;
				}
				else if (message.readOrWrite == 0) { //read
					//increment the clock for read
					sClock[1] += 10;
					frames[PCB[currentProcess].pageTable[message.pageNumber]].referenceBit = 1;
				}
			}
			
                	

        	}
		else { //process terminating
			
			if (writes < 10000) {
				fprintf(f, "Process %d terminating\n", message.pid);
				writes += 1;
			}
			
			//set corresponding PCB process id to 0
			for (i = 0; i != MAX_PROCESSES; i += 1) {
				if (PCB[i].processID == message.pid)
					break;
			}
			PCB[i].processID = 0;
			
			int tmp;
			//deallocate all frames
			for (j = 0; j != 32; j += 1) {
				//if page is in frame
				if (PCB[i].pageTable[j] >= 0) {
					//sift through memory frames to free frames used by this process
					int y = 0;
					while (true && y <= memoryReferenceQueue->size) {
						//dequeue item
						tmp = dequeue(memoryReferenceQueue);
						//dequeue returns MIN_INT on error
						if (tmp < 0)
							fprintf(f, "Queue is empty. Issue in termination code\n");
						//if frame number from queue corresponds to frame number in the page
						else if (tmp == PCB[i].pageTable[j]) {
							//printf("removing page %d from frame %d\n", j, tmp);
							frames[tmp].allocated = 0;
							break;
						}
						//otherwise put back into queue
						else { 
							enqueue(memoryReferenceQueue, tmp);
						}
						y += 1;
						
					}
					//clear page table 
					PCB[i].pageTable[j] = -1;
				}
				
			}
			
			//decrement process counter
			processCounter -= 1;
			
		}
		
		//every 100 memory accesses print frame table
		if (memoryAccesses % 100 == 0) {
			
			for (i = 0; i != 2; i += 1) {
				if ((i == 0) && (writes < 10000)) {
					for (j = 0; j != 256; j += 1) {
						if (frames[j].allocated == 0)
							fprintf(f, ".");
						else if (frames[j].dirtyBit == 1)
							fprintf(f, "D");
						else
							fprintf(f, "U");
					}
					fprintf(f, "\n");
				}
				if ((i == 1) && (writes < 10000)) {
					for (j = 0; j != 256; j += 1) {
						fprintf(f, "%d", frames[j].referenceBit);
					}
					fprintf(f, "\n");
				}
			}
	
		}
			
		message.mesgType = message.pid;

		if (msgsnd(msgid, &message, sizeof(message), 0) < 0)
			perror("msgsnd oss: ");
	}	
		

		
		
	
	//Destroy message queue
	
	fclose(f);	

	if (msgctl(msgid, IPC_RMID, NULL) < 0)
                perror("msgctl oss: ");
	if (shmctl(shmid, IPC_RMID, NULL) < 0)
		perror("shmctl oss: ");	
	if (shmdt(PCB) < 0)
		perror("shmdt oss: ");		
	return 0;
}
