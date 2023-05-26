#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/sem.h>
#include "merce.h"

int main (int argc, char * argv[]) {
	struct mesg_buffer {
    	long mesg_type;
    	char mesg_text[100];
	};

	struct mesg_buffer message; 
	int port_id;
	int banks;
	int shm_id_aval, shm_id_req, sem_id;
	struct merce *shm_ptr_aval, *shm_ptr_req;
	key_t mem_key;
	struct sembuf sops;
	struct merce *available;
	struct merce *requested;
	struct position pos;

	if((int) (shm_id_aval = atoi(argv[1])) < 0) {
		printf("*** shmget error porto***\n");
		exit(1);
	}
	if((struct merce *) (shm_ptr_aval = (struct merce *) shmat(shm_id_aval, NULL, 0)) == -1) {
		printf("*** shmat error porto***\n");
		exit(1);
	}
	if((int) (shm_id_req = atoi(argv[8])) < 0) {
		printf("*** shmget error porto***\n");
		exit(1);
	}
	if((struct merce *) (shm_ptr_req = (struct merce *) shmat(shm_id_req, NULL, 0)) == -1) {
		printf("*** shmat error porto***\n");
		exit(1);
	}

	sem_id = atoi(argv[2]);
	port_id = argv[4];
	banks = atoi(argv[7]);
	//printf("POSIZIONE PORTO %d = %s %s\n", atoi(argv[4]), argv[5], argv[6]);

	for(int i = 0; i < 3; i++) {
		sops.sem_op = -1;
		semop(sem_id, &sops, 1);

        //printf("PORTO %d HAS: %d OF %d\n", atoi(argv[4]), shm_ptr_aval[i].qty, i);
        //printf("PORTO %d REQUESTS: %d OF %d\n", atoi(argv[4]), shm_ptr_req[i].qty, i);

		sleep(0);

		sops.sem_op = 1;
		semop(sem_id, &sops, 1);
	}
	while(1) {
		msgrcv(atoi(argv[3]), &message, (sizeof(long) + sizeof(char) * 100), 1, 0);
		printf("RECEIVED : %s\n", message.mesg_text);
	}

	exit(0);
}