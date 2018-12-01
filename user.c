#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <sys/msg.h>
#include <time.h>
#include <signal.h>

struct mesgBuffer {
        long mesgType;
        char mesg[20];
} message;
 
int main(int argc, char ** argv) {
	
	//loop counters
	int i, j;
	
	key_t msgKey = ftok("msgqfile", 51);	
	
	int msgid = msgget(msgKey, 0);

	if (msgid == -1)
		perror("msgget user\n");

	if (msgrcv(msgid, &message, sizeof(message), 100, 0) < 0)
		perror("msgrcv user: \n");
	
	printf("%s\n", message.mesg);
			
	//seeding random
	srand(time(NULL));


	return 0;
}
