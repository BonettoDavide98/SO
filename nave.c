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
	double speed;
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
	int cargocapacity = 20;
	int cargocapacity_free = cargocapacity;

	//initialize cargo
	for(int c = 0; c < 20; c++) {
		cargo[c].type = 0;
		cargo[c].qty = 0;
	}

	sscanf(argv[5], "%lf", &speed);

	while(1) {
		sleep(1);
		/*printf("SHIP CARGO START LOOP: |");
			for(int i = 0; i < 20; i++) {
				if(cargo[i].qty > 0 && cargo[i].type > 0) {
					printf(" %d TONS OF %d POS: %d |", cargo[i].qty, cargo[i].type, i);
				}
			}
		printf("\n");*/

		message.mesg_type = 1;
		strcpy(string_out, argv[2]);
		strcat(string_out, ":");
		strcat(string_out, posx_str);
		strcat(string_out, ":");
		strcat(string_out, posy_str);
		strcat(string_out, ":");
		sprintf(text, "%d", getLargestCargo(cargo));
		strcat(string_out, text);
		strcpy(message.mesg_text, string_out);
		msgsnd(atoi(argv[6]), &message, (sizeof(long) + sizeof(char) * 100), 0);

		msgrcv(atoi(argv[1]), &message, (sizeof(long) + sizeof(char) * 100), 1, 0);
		printf("NAVE %s RECEIVED : %s\n", argv[2], message.mesg_text);
		if(strcmp(message.mesg_text, "terminate") == 0) {
			printf("NO MORE PORTS FOUND, TERMINATING NAVE %d\n", argv[2]);
			break;
		}


		strcpy(msgq_id_porto, strtok(message.mesg_text, ":"));
		strcpy(destx, strtok(NULL, ":"));
		strcpy(desty, strtok(NULL, ":"));
		printf("NAVE %s SETTING COURSE TO %s %s\n", argv[2], destx, desty);
		sscanf(destx, "%lf", &dest.x);
		sscanf(desty, "%lf", &dest.y);
		//printf("TIME: %f\n", (sqrt(pow((dest.x - pos.x),2) + pow((dest.y - pos.y),2)) / speed));
		//printf("TRAVELTIME: %ld\n", (long) ((sqrt(pow((dest.x - pos.x),2) + pow((dest.y - pos.y),2)) / speed * 1000000000)));
		traveltime = (long) ((sqrt(pow((dest.x - pos.x),2) + pow((dest.y - pos.y),2)) / speed * 1000000000));
		tv1.tv_nsec = traveltime % 1000000000;
		tv1.tv_sec = (int) ((traveltime - tv1.tv_nsec) / 1000000000);
		printf("SEC: %d NANOSEC %ld\n", tv1.tv_sec, tv1.tv_nsec);
		nanosleep(&tv1, &tv2);
		pos.x = dest.x;
		pos.y = dest.y;
		strcpy(posx_str, destx);
		strcpy(posy_str, desty);
		printf("NAVE %s ARRIVED AT %f %f\n", argv[2], pos.x, pos.y);
		
		strcpy(message.mesg_text, "dockrq");
		strcat(message.mesg_text, ":");
		strcat(message.mesg_text, argv[1]);
		msgsnd(atoi(msgq_id_porto), &message, (sizeof(long) + sizeof(char) * 100), 0);

		msgrcv(atoi(argv[1]), &message, (sizeof(long) + sizeof(char) * 100), 1, 0);
		strcpy(text, strtok(message.mesg_text, ":"));
		strcpy(shm_id_porto_req, strtok(NULL, ":"));
		strcpy(shm_id_porto_aval, strtok(NULL, ":"));
		if(strcmp(text, "accept") == 0) {
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
								printf("OFFLOADING %d TO FULFILL REQUEST OF %d TONS OF %d\n", shm_ptr_porto_req[i].qty, shm_ptr_porto_req[i].qty, cargo[k].type);
								cargo[k].qty -= shm_ptr_porto_req[i].qty;
								shm_ptr_porto_req[i].qty = 0;
								if(cargo[k].qty == 0) {
									cargo[k].type = 0;
								}
							} else {
								printf("OFFLOADING %d TO PARTIALLY FULFILL REQUEST OF %d TONS OF %d\n", cargo[k].qty, shm_ptr_porto_req[i].qty, cargo[k].type);
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
			printf("FREE CARGO: %d\n", cargocapacity_free);

			for(int i = 0; i < 50 && cargocapacity_free > 0; i++) {
				if(shm_ptr_porto_aval[i].type > 0 && shm_ptr_porto_aval[i].qty > 0) {
					if(cargocapacity_free >= shm_ptr_porto_aval[i].qty) {
						for(int j = 0; j < 20; j++) {
							if(cargo[j].qty == 0 && cargo[j].type == 0) {
								printf("LOADING %d TONS OF %d\n", shm_ptr_porto_aval[i].qty, shm_ptr_porto_aval[i].type);
								cargo[j].type = shm_ptr_porto_aval[i].type;
								cargo[j].qty = shm_ptr_porto_aval[i].qty;
								cargocapacity_free -= cargo[j].qty;
								shm_ptr_porto_aval[i].type = 0;
								shm_ptr_porto_aval[i].qty = 0;
								j = 20;
							}
						}
					} else {
						for(int j = 0; j < 20; j++) {
							if(cargo[j].qty == 0 && cargo[j].type == 0) {
								printf("LOADING %d OF %d\n", cargocapacity_free, shm_ptr_porto_aval[i].type);
								cargo[j].type = shm_ptr_porto_aval[i].type;
								shm_ptr_porto_aval[i].qty -= cargocapacity_free;
								cargo[j].qty = cargocapacity_free;
								cargocapacity_free = 0;
								printf("REMAINING: %d\n", shm_ptr_porto_aval[i].qty);
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

			printf("SHIP CARGO: |");
			for(int i = 0; i < 20; i++) {
				if(cargo[i].qty > 0 && cargo[i].type > 0) {
					printf(" %d TONS OF %d POS: %d |", cargo[i].qty, cargo[i].type, i);
				}
			}
			printf("\n");
		}
	}

	exit(0);
}

int getLargestCargo(struct merce * cargo) {
	int max = 0;
	int imax = 0;
	for (int i = 0; i < 20; i++) {
		if(cargo[i].type > 0 && cargo[i].qty > max) {
			max = cargo[i].qty;
			imax = cargo[i].type;
		}
	}

	if(imax >= 0) {
		return imax;
	}
}