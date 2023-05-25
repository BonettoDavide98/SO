#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <string.h>
#include "merce.h"

#define NAVE "nave.o"
#define PORTO "porto.o"

int main () {
	struct mesg_buffer {
    	long mesg_type;
    	char mesg_text[100];
	};

	//DA RENDERE LETTO DA FILE
	int numporti = 2;
	int nummerci = 3;
	int maxrandommerce = 10;
	int numnavi = 2;
	int banchine = 2;
	double max_x = 10;
	double max_y = 10;
	int shipsize = 10;
	double shipspeed = 1;
	struct mesg_buffer message;

	int sem_id, status;
	pid_t child_pid, *kid_pids;
	struct sembuf sops;
    char *args[8];
	char *argss[8];
	int *ports_shm_id_aval = malloc(numporti * sizeof(ports_shm_id_aval));
	struct merce **ports_shm_ptr_aval = malloc(numporti * sizeof(ports_shm_ptr_aval));
	struct position *ports_positions = malloc(numporti * sizeof(struct position *));
    char shm_id_str[3*sizeof(int)+1];
    char sem_id_str[3*sizeof(sem_id)+1];
    char msgq_id_str[3*sizeof(int)+1];
	char id[2];
	char x[20];
	char y[20];
	char banks[20];
	char speed[20];
	int master_msgq;
	char master_msgq_str[3*sizeof(int)+1];

	int *msgqueue = malloc(numporti * sizeof(int));

	srand(time(NULL));

	sem_id = semget(IPC_PRIVATE, 1, 0600);
	semctl(sem_id, 0, SETVAL, 0);

	sops.sem_num = 0;
	sops.sem_flg = 0;

	kid_pids = malloc(numnavi * sizeof(*kid_pids));

	sops.sem_op = 1;
	semop(sem_id, &sops, 1);

	if((master_msgq = msgget(IPC_PRIVATE, IPC_CREAT | 0600)) == -1) {
		printf("*** master msgqueue error ***\n");
		exit(1);
	}

	int i = 0;
	//create ports
	/*for(i = 0; i < numporti; i++) {
		if((int) (ports_shm_id_aval[i]  = shmget(IPC_PRIVATE, nummerci * sizeof(struct merce *), 0600)) < 0) {
			printf("*** shmget error ***\n");
			exit(1);
		}
		if((struct merce *) (ports_shm_ptr_aval[i] = (struct merce *) shmat(ports_shm_id_aval[i], NULL, 0)) == -1) {
			printf("*** shmat error ***\n");
			exit(1);
		}
		if((msgqueue[i] = msgget(IPC_PRIVATE, IPC_CREAT | 0600)) == -1) {
			printf("*** msgqueue error ***\n");
			exit(1);
		}
		
    	ports_positions[i].x = (rand() / (double)RAND_MAX) * max_x;
    	ports_positions[i].y = (rand() / (double)RAND_MAX) * max_y;
		
		printf("PORT CREATED IN %f %f\n", ports_positions[i].x, ports_positions[i].y);

		for(int j = 0; j < nummerci; j++) {
			ports_shm_ptr_aval[i][j].qty = rand() % maxrandommerce;
			printf("ADDED: %d TO PORT %d\n" , ports_shm_ptr_aval[i][j].qty, i);
		}

		sprintf(shm_id_str, "%d", ports_shm_id_aval[i]);
		sprintf(sem_id_str, "%d", sem_id);
		sprintf(msgq_id_str, "%d", msgqueue[i]);
		sprintf(id, "%d", i);
		sprintf(x, "%f", ports_positions[i].x);
		sprintf(y, "%f", ports_positions[i].y);
		sprintf(banks, "%d", banchine);
		args[0] = PORTO;
		args[1] = shm_id_str;
		args[2] = sem_id_str;
		args[3] = msgq_id_str;
		args[4] = id;
		args[5] = x;
		args[6] = y;
		args[7] = banks;
    	args[8] = NULL;

		switch(kid_pids[i] = fork()) {
			case -1:
				break;
			case 0:
    			execve(PORTO, args, NULL);
				break;
			default:
				break;
		}
	}*/

	for(int j = 0; j < numnavi; j++) {
		if((msgqueue[i+j] = msgget(IPC_PRIVATE, IPC_CREAT | 0600)) == -1) {
			printf("*** msgqueue error ***\n");
			exit(1);
		}
		sprintf(msgq_id_str, "%d", msgqueue[i+j]);
		sprintf(master_msgq_str, "%d", master_msgq);
		sprintf(id, "%d", j);
		sprintf(x, "%f", (rand() / (double)RAND_MAX) * max_x);
		sprintf(y, "%f", (rand() / (double)RAND_MAX) * max_y);
		sprintf(speed, "%f", shipspeed);
		argss[0] = NAVE;
		argss[1] = msgq_id_str;
		argss[2] = id;
		argss[3] = x;
		argss[4] = y;
		argss[5] = speed;
		argss[6] = master_msgq_str;
		argss[7] = NULL;

		printf("TEST: %s %s\n", x, y);
		
		switch(kid_pids[i+j] = fork()) {
			case -1:
				break;
			case 0:
    			execve(NAVE, argss, NULL);
				break;
			default:
				break;
		}
	}

	char idin[10];
	char posx_str[20];
	char posy_str[20];
	char merce[20];
	
	int l = 0;
	while(l < 20) {
		msgrcv(master_msgq, &message, (sizeof(long) + sizeof(char) * 100), 1, 0);
		printf("MASTER RECEIVED : %s\n", message.mesg_text);
		strcpy(idin, strtok(message.mesg_text, ","));
		strcpy(posx_str, strtok(NULL, ","));
		strcpy(posy_str, strtok(NULL, ","));
		strcpy(merce, strtok(NULL, ","));
		printf("PARSED ID: %s, POSX: %s, POSY: %s, MERCE: %s\n", id, posx_str, posy_str, merce);

		message.mesg_type = 1;
		strcpy(message.mesg_text, "PROVA INVIO");
		msgsnd(msgqueue[0], &message, (sizeof(long) + sizeof(char) * 100), 0);
		msgsnd(msgqueue[1], &message, (sizeof(long) + sizeof(char) * 100), 0);
		l++;
	}

	while((child_pid = wait(&status)) != -1) {
		dprintf(2, "Pid=%d. Sender (PID=%d) terminated with status 0x%04X\n", getpid(), child_pid, status);
	}
	msgctl(msgqueue[0], IPC_RMID, NULL);
	msgctl(msgqueue[1], IPC_RMID, NULL);

	/*for(int i = 0; i < numporti; i++) {
		for(int j = 0; j < nummerci; j++) {
			printf("END: %d\n" , ports_shm_ptr_aval[i][j].qty);
		}
	}*/

	semctl(sem_id, 0, IPC_RMID);
	shmctl(ports_shm_id_aval, IPC_RMID, NULL);

	exit(0);
}

static int getRequesting(struct merce ** portscontents, int merce, int nporti) {
	for(int i = 0; i < nporti; i++) {
		if(portscontents[i][merce].qty > 0) {
			return i;
		}
	}
	return -1;
}
