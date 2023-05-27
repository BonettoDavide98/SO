#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "merce.h"

int main (int argc, char * argv[]) {
	struct mesg_buffer {
    	long mesg_type;
    	char mesg_text[100];
	};
	struct mesg_buffer message;
	struct position pos;
	double speed = atoi(argv[5]);
	char posx_str[20];
	strcpy(posx_str, argv[3]);
	char posy_str[20];
	strcpy(posy_str, argv[4]);
	sscanf(argv[3], "%lf", &pos.x);
	sscanf(argv[4], "%lf", &pos.y);
	char string_out[100];

	char msgq_id_porto[20];
	char shm_id_porto_req[20];
	struct merce *shm_ptr_porto_req;
	char shm_id_porto_aval[20];
	struct merce *shm_ptr_porto_aval;
	char destx[20];
	char desty[20];
	struct position dest;
	struct timespec tv1, tv2;
	long traveltime;
	char text[20];

	struct merce cargo[20];
	int cargocapacity = atoi(argv[7]);
	int cargocapacity_free = cargocapacity;

	//initialize cargo
	for(int c = 0; c < 20; c++) {
		cargo[c].type = 0;
		cargo[c].qty = 0;
	}

	int randomportflag = 0;
	message.mesg_type = 1;

	//ship loop, will last until interrupted by an external process
	while(1) {
		//ask master the closest port that asks for my largest merce
		sleep(1);
		removeSpoiled(cargo, atoi(argv[2]));
		strcpy(message.mesg_text, argv[2]);
		strcat(message.mesg_text, ":");
		strcat(message.mesg_text, posx_str);
		strcat(message.mesg_text, ":");
		strcat(message.mesg_text, posy_str);
		strcat(message.mesg_text, ":");
		if(randomportflag == 0) {
			sprintf(text, "%d", getLargestCargo(cargo));
			strcat(message.mesg_text, text);
		} else {
			randomportflag = 0;
			strcat(message.mesg_text, "0");
		}
		msgsnd(atoi(argv[6]), &message, (sizeof(long) + sizeof(char) * 100), 0);

		msgrcv(atoi(argv[1]), &message, (sizeof(long) + sizeof(char) * 100), 1, 0);
		printf("SHIP %s RECEIVED : %s\n", argv[2], message.mesg_text);

		strcpy(msgq_id_porto, strtok(message.mesg_text, ":"));
		strcpy(destx, strtok(NULL, ":"));
		strcpy(desty, strtok(NULL, ":"));
		sscanf(destx, "%lf", &dest.x);
		sscanf(desty, "%lf", &dest.y);
		traveltime = (long) ((sqrt(pow((dest.x - pos.x),2) + pow((dest.y - pos.y),2)) / speed * 1000000000));
		tv1.tv_nsec = traveltime % 1000000000;
		tv1.tv_sec = (int) ((traveltime - tv1.tv_nsec) / 1000000000);
		printf("SHIP %s SETTING COURSE TO %s %s, ETA: %d,%ld DAYS\n", argv[2], destx, desty, tv1.tv_sec, tv1.tv_nsec);
		nanosleep(&tv1, &tv2);
		pos.x = dest.x;
		pos.y = dest.y;
		strcpy(posx_str, destx);
		strcpy(posy_str, desty);
		printf("SHIP %s ARRIVED AT PORT IN %f %f, SENDING DOCKING REQUEST ...\n", argv[2], pos.x, pos.y);
		
		strcpy(message.mesg_text, "dockrq");
		strcat(message.mesg_text, ":");
		strcat(message.mesg_text, argv[1]);
		msgsnd(atoi(msgq_id_porto), &message, (sizeof(long) + sizeof(char) * 100), 0);

		msgrcv(atoi(argv[1]), &message, (sizeof(long) + sizeof(char) * 100), 1, 0);
		strcpy(text, strtok(message.mesg_text, ":"));
		strcpy(shm_id_porto_req, strtok(NULL, ":"));
		strcpy(shm_id_porto_aval, strtok(NULL, ":"));
		if(strcmp(text, "accept") == 0) {
			removeSpoiled(cargo, atoi(argv[2]));
			if((struct merce *) (shm_ptr_porto_req = (struct merce *) shmat(atoi(shm_id_porto_req), NULL, 0)) == -1) {
				printf("*** shmat error nave req ***\n");
				exit(1);
			}
			if((struct merce *) (shm_ptr_porto_aval = (struct merce *) shmat(atoi(shm_id_porto_aval), NULL, 0)) == -1) {
				printf("*** shmat error nave aval ***\n");
				exit(1);
			}

			for(int k = 0; k < 20; k++) {
				if(cargo[k].type > 0 && cargo[k].qty > 0) {
					for(int i = 0; i < 50 && shm_ptr_porto_req[i].type > 0; i++) {
						if(cargo[k].type == shm_ptr_porto_req[i].type) {
							if(cargo[k].qty >= shm_ptr_porto_req[i].qty) {
								printf("SHIP %s OFFLOADING %d TO FULFILL REQUEST OF %d TONS OF %d\n", argv[2], shm_ptr_porto_req[i].qty, shm_ptr_porto_req[i].qty, cargo[k].type);
								cargo[k].qty -= shm_ptr_porto_req[i].qty;
								shm_ptr_porto_req[i].qty = 0;
								if(cargo[k].qty == 0) {
									cargo[k].type = 0;
								}
							} else {
								printf("SHIP %s OFFLOADING %d TO PARTIALLY FULFILL REQUEST OF %d TONS OF %d\n",  argv[2], cargo[k].qty, shm_ptr_porto_req[i].qty, cargo[k].type);
								shm_ptr_porto_req[i].qty -= cargo[k].qty;
								cargo[k].qty = 0;
								cargo[k].type = 0;
							}
						}
					}
				}
			}

			cargocapacity_free = cargocapacity;
			for(int i = 0; i < 20; i++) {
				if(cargo[i].type > 0 && cargo[i].qty > 0) {
					cargocapacity_free = cargocapacity_free - cargo[i].qty;
				}
			}

			for(int i = 0; i < 50 && cargocapacity_free > 0; i++) {
				if(shm_ptr_porto_aval[i].type > 0 && shm_ptr_porto_aval[i].qty > 0) {
					if(cargocapacity_free >= shm_ptr_porto_aval[i].qty) {
						for(int j = 0; j < 20; j++) {
							if(cargo[j].qty == 0 && cargo[j].type == 0) {
								printf("SHIP %s LOADING %d TONS OF %d\n", argv[2], shm_ptr_porto_aval[i].qty, shm_ptr_porto_aval[i].type);
								cargo[j].type = shm_ptr_porto_aval[i].type;
								cargo[j].qty = shm_ptr_porto_aval[i].qty;
								cargo[j].spoildate.tv_sec = shm_ptr_porto_aval[i].spoildate.tv_sec;
								cargo[j].spoildate.tv_usec = shm_ptr_porto_aval[i].spoildate.tv_usec;
								cargocapacity_free -= cargo[j].qty;
								shm_ptr_porto_aval[i].type = 0;
								shm_ptr_porto_aval[i].qty = 0;
								j = 20;
							}
						}
					} else {
						for(int j = 0; j < 20; j++) {
							if(cargo[j].qty == 0 && cargo[j].type == 0) {
								printf("SHIP %s LOADING %d OF %d\n",  argv[2], cargocapacity_free, shm_ptr_porto_aval[i].type);
								cargo[j].type = shm_ptr_porto_aval[i].type;
								shm_ptr_porto_aval[i].qty -= cargocapacity_free;
								cargo[j].qty = cargocapacity_free;
								cargo[j].spoildate.tv_sec = shm_ptr_porto_aval[i].spoildate.tv_sec;
								cargo[j].spoildate.tv_usec = shm_ptr_porto_aval[i].spoildate.tv_usec;
								cargocapacity_free = 0;
								j = 20;
							}
						}
					}
				}
			}


			sleep(1);

			strcpy(message.mesg_text, "dockfree");
			strcat(message.mesg_text, ":");
			strcat(message.mesg_text, argv[2]);
			msgsnd(atoi(msgq_id_porto), &message, (sizeof(long) + sizeof(char) * 100), 0);

			printf("SHIP %s CARGO: |", argv[2]);
			for(int i = 0; i < 20; i++) {
				if(cargo[i].qty > 0 && cargo[i].type > 0) {
					printf(" %d TONS OF %d |", cargo[i].qty, cargo[i].type);
				}
			}
			printf("\n");
		} else {
			printf("SHIP %s HAS BEEN DENIED DOCKING BECAUSE THE QUEUE WAS TOO LONG");
			randomportflag = 1;
		}
	}

	exit(0);
}

int getLargestCargo(struct merce * cargo) {
	int label = -1;
	int temp = 0;
	int maxlabel;
	int max = -1;

	for (int i = 0; i < 20; i++) {
		if(cargo[i].type != maxlabel && cargo[i].type > 0 && cargo[i].qty > 0) {
			label = cargo[i].type;
			for(int j = i; j < 20; j++) {
				if(cargo[j].type == label && cargo[j].qty > 0) {
					temp += cargo[i].qty;
				}
			}

			if(temp > max) {
				maxlabel = label;
				max = temp;
			}
		}
	}

	return maxlabel;
}

void removeSpoiled(struct merce *available, int naveid) {
	struct timeval currenttime;
	gettimeofday(&currenttime, NULL);
	for(int i = 0; i < 20; i++) {
		if(available[i].type > 0 && available[i].qty > 0) {
			if(available[i].spoildate.tv_sec < currenttime.tv_sec) {
				printf("REMOVED %d TONS OF %d FROM SHIP %d DUE TO SPOILAGE\n", available[i].qty, available[i].type, naveid);
				available[i].type = 0;
				available[i].qty = 0;
			} else if(available[i].spoildate.tv_sec == currenttime.tv_sec) {
				if(available[i].spoildate.tv_usec <= currenttime.tv_usec) {
				printf("REMOVED %d TONS OF %d FROM SHIP %d DUE TO SPOILAGE\n", available[i].qty, available[i].type, naveid);
					available[i].type = 0;
					available[i].qty = 0;
				}
			}
		}
	}
}