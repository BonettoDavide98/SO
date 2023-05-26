#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <string.h>
#include <math.h>
#include "merce.h"

#define NAVE "nave.o"
#define PORTO "porto.o"

int main () {
	struct mesg_buffer {
    	long mesg_type;
    	char mesg_text[100];
	};

	//DA RENDERE LETTO DA FILE
	int numporti = 10;
	int nummerci = 3;
	int maxrandommerce = 10;
	int numnavi = 5;
	int banchine = 2;
	double max_x = 10;
	double max_y = 10;
	int shipsize = 10;
	double shipspeed = 1;
	char so_fill[1] = "5";
	struct mesg_buffer message;

	int sem_id, status;
	pid_t child_pid, *kid_pids;
	struct sembuf sops;
    char *args[11];
	char *argss[8];
	int *ports_shm_id_aval = malloc(numporti * sizeof(ports_shm_id_aval));
	struct merce **ports_shm_ptr_aval = malloc(numporti * sizeof(ports_shm_ptr_aval));
	int *ports_shm_id_req = malloc(numporti * sizeof(ports_shm_id_req));
	struct merce **ports_shm_ptr_req = malloc(numporti * sizeof(ports_shm_ptr_req));
	struct position *ports_positions = malloc(numporti * sizeof(struct position));
    char shm_id_str[3*sizeof(int)+1];
    char shm_id_req_str[3*sizeof(int)+1];
    char sem_id_str[3*sizeof(int)+1];
    char msgq_id_str[3*sizeof(int)];
	char id[2];
	char x[20];
	char y[20];
	char banks[20];
	char speed[20];
	int master_msgq;
	char master_msgq_str[3*sizeof(int)+1];

	int *msgqueue_porto = malloc(numporti * sizeof(int));
	int *msgqueue_nave = malloc(numnavi * sizeof(int));

	srand(time(NULL));

	sem_id = semget(IPC_PRIVATE, 1, 0600);
	semctl(sem_id, 0, SETVAL, 0);

	sops.sem_num = 0;
	sops.sem_flg = 0;

	kid_pids = malloc((numporti + numnavi) * sizeof(*kid_pids));

	if((master_msgq = msgget(IPC_PRIVATE, IPC_CREAT | 0600)) == -1) {
		printf("*** master msgqueue error ***\n");
		exit(1);
	}

	ports_positions[0].x = 0;
	ports_positions[0].y = 0;

	ports_positions[1].x = max_x;
	ports_positions[1].y = 0;

	ports_positions[2].x = 0;
	ports_positions[2].y = max_y;

	ports_positions[3].x = max_x;
	ports_positions[3].y = max_y;

	//create ports
	for(int i = 0; i < numporti; i++) {
		if((int) (ports_shm_id_aval[i]  = shmget(IPC_PRIVATE, nummerci * sizeof(struct merce *), 0600)) < 0) {
			printf("*** shmget aval error ***\n");
			exit(1);
		}

		ports_shm_ptr_aval[i] = malloc(50 * sizeof(struct merce));
		if((struct merce *) (ports_shm_ptr_aval[i] = (struct merce *) shmat(ports_shm_id_aval[i], NULL, 0)) == -1) {
			printf("*** shmat aval error ***\n");
			exit(1);
		}

		if((int) (ports_shm_id_req[i]  = shmget(IPC_PRIVATE, nummerci * sizeof(struct merce *), 0600)) < 0) {
			printf("*** shmget req error ***\n");
			exit(1);
		}

		ports_shm_ptr_req[i] = malloc(50 * sizeof(struct merce));
		if((struct merce *) (ports_shm_ptr_req[i] = (struct merce *) shmat(ports_shm_id_req[i], NULL, 0)) == -1) {
			printf("*** shmat req error ***\n");
			exit(1);
		}

		if((msgqueue_porto[i] = msgget(IPC_PRIVATE, 0600)) == -1) {
			printf("*** msgqueue_porto error ***\n");
			exit(1);
		}
		//printf("msgqueue_porto[%d] = %d\n", i, msgqueue_porto[i]);
		
		if(i > 3) {
			ports_positions[i].x = (rand() / (double)RAND_MAX) * max_x;
			ports_positions[i].y = (rand() / (double)RAND_MAX) * max_y;
		}
		
		printf("PORT CREATED IN %f %f\n", ports_positions[i].x, ports_positions[i].y);

		for(int j = 0; j < nummerci; j++) {
			int a = 0, b = 0;
			if(rand()%2 == 1) {
				ports_shm_ptr_aval[i][a].type = j + 1;
				ports_shm_ptr_aval[i][a].qty = rand() % maxrandommerce;
				printf("ADDED: %d TONS OF %d TO PORT %d\n" , ports_shm_ptr_aval[i][a].qty, ports_shm_ptr_aval[i][a].type, i);
				a = a + 1;
			} else {
				ports_shm_ptr_req[i][b].type = j + 1;
				ports_shm_ptr_req[i][b].qty = rand() % maxrandommerce;
				printf("ADDED REQUEST: %d TONS OF %d TO PORT %d\n" , ports_shm_ptr_req[i][b].qty, ports_shm_ptr_req[i][b].type, i);
				b = b + 1;
			}
		}
	}

	//create ships
	for(int j = 0; j < numnavi; j++) {
		if((msgqueue_nave[j] = msgget(IPC_PRIVATE, IPC_CREAT | 0600)) == -1) {
			printf("*** msgqueue error ***\n");
			exit(1);
		}
	}

	for(int i = 0; i < numporti; i++) {
		sprintf(shm_id_str, "%d", ports_shm_id_aval[i]);
		sprintf(shm_id_req_str, "%d", ports_shm_id_req[i]);
		sprintf(sem_id_str, "%d", sem_id);
		sprintf(msgq_id_str, "%d", msgqueue_porto[i]);
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
		args[8] = shm_id_req_str;
    	args[9] = so_fill;
    	args[10] = NULL;

		switch(kid_pids[i] = fork()) {
			case -1:
				break;
			case 0:
    			execve(PORTO, args, NULL);
				break;
			default:
				break;
		}
	}

	for(int j = 0; j < numnavi; j++) {
		sprintf(msgq_id_str, "%d", msgqueue_nave[j]);
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

		//printf("TEST: %s %s\n", x, y);
		
		switch(kid_pids[numporti+j] = fork()) {
			case -1:
				break;
			case 0:
    			execve(NAVE, argss, NULL);
				break;
			default:
				break;
		}
	}

	sops.sem_op = 1;
	semop(sem_id, &sops, 1);

	char idin[10];
	char posx_str[20];
	char posy_str[20];
	char merce[20];
	char string_out[100];
	int idfind;
	
	while(1) {
		msgrcv(master_msgq, &message, (sizeof(long) + sizeof(char) * 100), 1, 0);
		printf("MASTER RECEIVED : %s\n", message.mesg_text);
		strcpy(idin, strtok(message.mesg_text, ":"));
		strcpy(posx_str, strtok(NULL, ":"));
		strcpy(posy_str, strtok(NULL, ":"));
		strcpy(merce, strtok(NULL, ":"));
		printf("PARSED ID: %s, POSX: %s, POSY: %s, MERCE: %s\n", idin, posx_str, posy_str, merce);
		idfind = getRequesting(posx_str, posy_str, ports_positions, ports_shm_ptr_req, atoi(merce), numporti);

		message.mesg_type = 1;
		if(idfind < 0) {
			strcpy(message.mesg_text, "terminate");
			msgsnd(msgqueue_nave[atoi(idin)], &message, (sizeof(long) + sizeof(char) * 100), 0);
		}
		sprintf(x, "%f", ports_positions[idfind].x);
		sprintf(y, "%f", ports_positions[idfind].y);
		sprintf(msgq_id_str, "%d", msgqueue_porto[idfind]);
		strcpy(message.mesg_text, msgq_id_str);
		strcat(message.mesg_text, ":");
		strcat(message.mesg_text, x);
		strcat(message.mesg_text, ":");
		strcat(message.mesg_text, y);
		msgsnd(msgqueue_nave[atoi(idin)], &message, (sizeof(long) + sizeof(char) * 100), 0);
	}

	while((child_pid = wait(&status)) != -1) {
		dprintf(2, "Pid=%d. Sender (PID=%d) terminated with status 0x%04X\n", getpid(), child_pid, status);
	}

	for(int i = 0; i < numporti; i++) {
		msgctl(msgqueue_porto[i], IPC_RMID, NULL);
	}
	for(int i = 0; i < numnavi; i++) {
		msgctl(msgqueue_nave[i], IPC_RMID, NULL);
	}
	msgctl(master_msgq, IPC_RMID, NULL);

	/*for(int i = 0; i < numporti; i++) {
		for(int j = 0; j < nummerci; j++) {
			printf("END: %d\n" , ports_shm_ptr_aval[i][j].qty);
		}
	}*/

	semctl(sem_id, 0, IPC_RMID);
	shmctl(ports_shm_id_aval, IPC_RMID, NULL);

	exit(0);
}

int getRequesting(char *posx_s, char *posy_s, struct position * portpositions, struct merce ** portsrequests, int merce, int nporti) {
	struct position currpos;
	struct position minpos;
	minpos.x = 1000000;
	minpos.y = 1000000;
	sscanf(posx_s, "%lf", &currpos.x);
	sscanf(posy_s, "%lf", &currpos.y);
	int imin = -1;
	for(int i = 0; i < nporti; i++) {
		for(int j = 0; j < 50; j++) {
			if(portsrequests[i][j].type == merce && portsrequests[i][j].qty > 0) {
				if(sqrt(pow((portpositions[i].x - currpos.x),2) + pow((portpositions[i].y - currpos.y),2)) < sqrt(pow((minpos.x - currpos.x),2) + pow((minpos.y - currpos.y),2))) {
					imin = i;
					minpos.x = portpositions[i].x;
					minpos.y = portpositions[i].y;
				}
			}
		}
	}
	if(imin >= 0) {
		printf("FOUND CLOSEST PORT %d REQUESTING MERCE %d\n", imin, merce);
		return imin;
	}
	printf("NO REQUESTS OF MERCE %d, RETURNING RANDOM PORT\n", merce);
	return rand() % nporti;
}
