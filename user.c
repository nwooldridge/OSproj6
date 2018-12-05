#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <sys/msg.h>
#include <time.h>
#include <signal.h>

struct mesgBuffer {
	//store pid in mesgType
        long mesgType;
	//process id
	pid_t pid;
	//page number
	int pageNumber;
	//0 for read, 1 for write
	int readOrWrite;
	

} message;
 
int main(int argc, char ** argv) {
	
	//loop counters
	int i, j;
	
	//seeding random
	srand(time(NULL));

	//setting up  message queues
	key_t msgKey = ftok("msgqfile", 51);	

	if (msgKey == -1)
		perror("ftok user: ");

	int msgid = msgget(msgKey, 0);
 

	if (msgid == -1)
		perror("msgget user\n");

	/*
	if (msgrcv(msgid, &message, sizeof(message), getpid(), 0) < 0)
		perror("msgrcv user: \n");
	*/

	//seeding random
	srand(time(NULL));
	
	
	int loopBreak = 1;
	int count = 0;
	while (loopBreak) {
		
		message.mesgType = 1;

        	message.pid = getpid();

        	message.pageNumber = rand() % 32;

        	message.readOrWrite = rand() % 2;

        	if (msgsnd(msgid, &message, sizeof(message), 0) < 0)
                	perror("msgsnd user:");
	
		if (msgrcv(msgid, &message, sizeof(message), getpid(), 0) < 0)
			perror("msgsnd user: ");
		
		count += 1;
		
	}
		
	
	
	
		

	return 0;
}
