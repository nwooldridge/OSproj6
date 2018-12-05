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

	//simulated clock
	unsigned int sClock[2];	
	sClock[0] = 0; sClock[1] = 0;
	
	//keeps track of amount of active processes
	int processCounter = 0;
	//keeps track of total page faults 
	int pageFaults = 0;
	
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
			continue;
		}
	
	
		//if enough time has passed to fork
		if ((sClock[0] >= nextFork[0]) && (sClock[1] >= nextFork[1]) && (processCounter < 18)) {
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
                                		printf("creating process with pid %d and assigning it to P%d", getpid(), i);
                                		processCounter += 1;
						break;
                        		}
                		}		
				
				//executes user process
				execl("./user", "user", NULL);
				//if it makes it this far, its an error
                		perror("execlp: ");
	
			}
			
			//increase time to next fork by random value between 1 and 500 milliseconds
			nextFork[1] = pow(10, 6) * (rand() % 500 + 1);
	
			//for every billion nanoseconds add 1 second and subtract billion nanoseconds
			while (nextFork[1] > 99999999) {
				nextFork[1] -= 1000000000;
				nextFork[0] += 1;
			}	
			while (sClock[1] > 999999999) {
				sClock[1] -= 1000000000;
				sClock[0] += 1;
			}
	
		}
	
		//receive message from user processes
		if (msgrcv(msgid, &message, sizeof(message), 1, 0) < 0)
               		perror("msgrcv: ");

		//if message is read or write request
		if (message.readOrWrite == 0 || message.readOrWrite == 1) { 
                	
			if (message.readOrWrite == 0)
				printf("Process %d requesting read to page %d at time %d : %d\n", message.pid, message.pageNumber, sClock[0], sClock[1]);
                	else if (message.readOrWrite == 1)
				printf("Process %d requesting write to page %d at time %d : %d\n", message.pid, message.pageNumber, sClock[0], sClock[1]);
                	else
				printf("error with something\n");
		
			//find pcb with corresponding process id
                	for (i = 0; i != MAX_PROCESSES; i += 1)
                		if (PCB[i].processID == message.pid)
                			break;

			//stores temporary values from queue
			int tmp;
	
			//used to keep track of what process we are working with
			int currentProcess = i;

			//if page requested isnt in a frame
			if (PCB[i].pageTable[message.pageNumber] == -1) {

                        	printf("page %d not in frame, page fault\n", message.pageNumber);
				pageFaults += 1;

				//if queue is full then oss performs swap of pages
				if (isFull(memoryReferenceQueue)) {
	
					//find frame to overwrite new page onto
					for (i = 0; i != memoryReferenceQueue->size; i += 1) {

						tmp = dequeue(memoryReferenceQueue);
						//dequeue returns small int on failure
						if (tmp < 0)
							printf("we've got issues\n");
						else {
							//if reference bit is 1, then move onto next in queue
							if (frames[tmp].referenceBit == 1) {
								enqueue(memoryReferenceQueue, tmp);
							}

							//else dequeue and change frame to new page
							else if (frames[tmp].referenceBit == 0) {
								
								//setting previous page back to -1 in page table
								//this requires searching through PCB's and finding corresponding
								//process
								for (j = 0; j != MAX_PROCESSES; j += 1) {
									if (PCB[j].processID == frames[tmp].processID)
										break;
								}
								for (k = 0; k != 32; k += 1)
									if (PCB[j].pageTable[k] == tmp)
										break;
								printf("swapping page %d into frame %d\n", message.pageNumber, tmp);
								PCB[j].pageTable[k] = -1;

								//store pid into frame
								frames[tmp].processID = message.pid;
								//store back in queue
								enqueue(memoryReferenceQueue, tmp);
								//setting frame number in page
								PCB[currentProcess].pageTable[message.pageNumber] = tmp;
								break;
							}
							else
								printf("weve got issues\n");
						}
					}
				}
				//if queue is not full
				else {
					//random frame to start looking for unallocated frames
					int x = rand() % 256;
					int count = 0;
					while (true) {
						//avoid overflow
						if (x > 255)
							x -= 255;
						//if loop iterates over 255 times, there is issues
						if (count > 255) {
							printf("issues here\n");
							break;
						}
						//starts with random frame, if it is allocated, then checks the next after
						//that and so on until an unallocated frame is found
						
						if (frames[x].allocated == 1)
							x += 1;
						else if (frames[x].allocated == 0) {
							
							//set this frame to allocated
							frames[x].allocated = 1;
							//set page to frame in pagetable
							PCB[currentProcess].pageTable[message.pageNumber] = x;
							if (message.readOrWrite == 1)
								frames[x].dirtyBit = 1;
							enqueue(memoryReferenceQueue, x);
							printf("adding page %d to frame %d\n", message.pageNumber, x);
							break;
						}
						count += 1;
					}
				}
                	}
			//otherwise the page is already in a frame
			else {
				printf("page %d already in frame %d\n", message.pageNumber, PCB[currentProcess].pageTable[message.pageNumber]);
				if (message.readOrWrite == 1) { //write
					//set dirty bit and increment clock
					frames[PCB[currentProcess].pageTable[message.pageNumber]].dirtyBit = 1;
					sClock[1] += 10;
				}
				else if (message.readOrWrite == 0) { //read
					//increment the clock for read
					sClock[1] += 10;
				}
			}
			
                	

        	}
		else { //process terminating
			printf("Process %d terminating\n", message.pid);
			
			//set corresponding PCB process id to 0
			for (i = 0; i != MAX_PROCESSES; i += 1)
				if (PCB[i].processID == message.pid)
					break;
			PCB[i].processID = 0;
			
			int tmp;
			//deallocate all frames
			for (j = 0; j != 32; j += 1) {
				//if page is in frame
				if (PCB[i].pageTable[j] >= 0) {
					//sift through memory frames to free frames used by this process
					while (true) {
						//dequeue item
						tmp = dequeue(memoryReferenceQueue);
						//dequeue returns MIN_INT on error
						if (tmp < 0)
							printf("Queue is empty. Issue in termination code\n");
						//if frame number from queue corresponds to frame number in the page
						else if (tmp == PCB[i].pageTable[j]) {
							printf("removing page %d from frame %d\n", j, tmp);
							frames[tmp].allocated = 0;
							break;
						}
						//otherwise put back into queue
						else { 
							enqueue(memoryReferenceQueue, tmp);
						}
					}
					//clear page table 
					PCB[i].pageTable[j] = -1;
				}
				
			}
			
			//decrement process counter
			processCounter -= 1;
			
		}
		
		
		message.mesgType = message.pid;

		if (msgsnd(msgid, &message, sizeof(message), 0) < 0)
			perror("msgsnd oss: ");
	}	
		

		
		
	
	//Destroy message queue
	if (msgctl(msgid, IPC_RMID, NULL) < 0)
                perror("msgctl oss: ");
	if (shmctl(shmid, IPC_RMID, NULL) < 0)
		perror("shmctl oss: ");	
	if (shmdt(PCB) < 0)
		perror("shmdt oss: ");		
	return 0;
}
