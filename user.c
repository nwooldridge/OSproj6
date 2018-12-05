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
	
		//mesgType set to 1 for oss process to receive message	
		message.mesgType = 1;
		
		//assign process id to message so oss knows 
        	message.pid = getpid();	

		//get random page number for user to simulate reading or writing to
        	message.pageNumber = rand() % 32;
		
		//0 is read 1 is write. if readOrWrite is -1, then user process is terminating
        	message.readOrWrite = rand() % 2;
		
		//every 1000 iterations of loop, process has 50% chance of termination
		if (count % 1000  == 0) {
			if (rand() % 2 == 0) {
				loopBreak = 0;
				message.readOrWrite = -1;
			}
		}

        	if (msgsnd(msgid, &message, sizeof(message), 0) < 0)
                	perror("msgsnd user:");
	
		if (msgrcv(msgid, &message, sizeof(message), getpid(), 0) < 0)
			perror("msgsnd user: ");
		
		count += 1;
		
	}
		
	
	
	
		

	return 0;
}
