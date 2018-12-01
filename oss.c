#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <sys/msg.h>
#include <time.h>
#include <signal.h>

//struct for keeping track of child processes
struct process {
	pid_t processID;
	//array to represent virtual address space
	int pageTable[32];
};
typedef struct process process;

//message queue structure
struct mesgBuffer {
        long mesgType;
        char mesg[20];
} message;

key_t msgKey;
int msgid;

int main(int argc, char ** argv) {
	//loop counters
	int i, j;

	//Create key for msgqueue
	msgKey = ftok("msgqfile", 51);
	
	//create message queue
	msgid = msgget(msgKey, 0666|IPC_CREAT);
	
	if (msgid == -1)
		perror("msgget oss: \n");
	
	//array to represent physical address space
	int frames[256];
	
	//0 in the array means the "frame" is unallocated
	for (i = 0; i != 256; i += 1) {
		frames[i] = 0;
	}
	
	message.mesgType = 100;
	
	strncpy(message.mesg, "hello there\0", sizeof(message.mesg));
	
	if (msgsnd(msgid, &message, sizeof(message), 0))
		perror("msgsnd oss");
	
	printf("%s\n", message.mesg);
		
	pid_t childpid = fork();
	
	if (childpid == -1)
		perror("fork: ");
	else if (childpid == 0) {
		//in child
		execlp("./user", "user", NULL);
		perror("execlp: ");
	}
	

	wait(0);
	
	
	//Destroy message queue
	if (msgctl(msgid, IPC_RMID, NULL) == -1)
                perror("msgctl oss: ");
		
	return 0;
}
