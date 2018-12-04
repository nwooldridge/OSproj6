#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <sys/msg.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <unistd.h>

//Queue structure
//Source: https://www.geeksforgeeks.org/queue-set-1introduction-and-array-implementation/
struct Queue 
{ 
    int front, rear, size; 
    unsigned capacity; 
    int* array; 
}; 

//create queue function
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
    printf("%d enqueued to queue\n", item); 
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
	//reference bit for second-chance FIFO page replacement algorithm
	int referenceBit;
	//dirty bit
	int dirtyBit;
	
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
}

//size of PCB
const int MAX_PROCESSES = 18;

int main(int argc, char ** argv) {
	//loop counters
	int i, j;

	//simulated clock
	unsigned int sClock[2];	
	sClock[0] = 0; sClock[1] = 0;
	
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
	
	//array to represent physical address space
	frame frames[256];
	
	//initialize frame table
	for (i = 0; i != 256; i += 1) {
		frames[i].referenceBit = 0;
		frames[i].dirtyBit = 0;
	}
	

	
		
	/*
	if (msgsnd(msgid, &message, sizeof(message), 0))
		perror("msgsnd oss");
	*/
	
		
	//fork off
	
	pid_t childpid = fork();
		
	if (childpid == -1)
		perror("fork: ");
	else if (childpid == 0) {
		//in child
	
		//set process id in PCB
		for (i = 0; i != MAX_PROCESSES; i += 1){
			printf("%d\n", i);
			if (PCB[i].processID == 0) {
				PCB[i].processID = getpid();
				printf("creating process with pid %d and assigning it to P%d", getpid(), i);
				break;
			}
		}
				
		execl("./user", "user", NULL);
		perror("execlp: ");
	}


	
	//receive message from user
	if (msgrcv(msgid, &message, sizeof(message), 1, 0) < 0)
		perror("msgrcv: ");
	
	if (message.readOrWrite == 0) { //read 
		printf("Process %d requesting read to page %d\n", message.pid, message.pageNumber);	
	}
	else {
		printf("Process %d requesting write to page %d\n", message.pid, message.pageNumber);
	}

		
		
	
	//Destroy message queue
	if (msgctl(msgid, IPC_RMID, NULL) == -1)
                perror("msgctl oss: ");
	
	if (shmctl(shmid, IPC_RMID, NULL) == -1)
		perror("shmctl oss: ");	
	if (shmdt(PCB) == -1)
		perror("shmdt oss: ");		
	return 0;
}
