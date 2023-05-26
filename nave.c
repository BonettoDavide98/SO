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

	int flag = 1;
	char msgq_id_porto[100];
	char destx[20];
	char desty[20];
	struct position dest;
	struct timespec tv1, tv2;

	sscanf(argv[5], "%lf", &speed);

	while(flag) {
		sleep(5);
		message.mesg_type = 1;
		strcpy(string_out, argv[2]);
		strcat(string_out, ":");
		strcat(string_out, posx_str);
		strcat(string_out, ":");
		strcat(string_out, posy_str);
		strcat(string_out, ":");
		strcat(string_out, "0");
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
		tv1.tv_sec = 0;
		tv1.tv_nsec = 10;//speed * 1000 * sqrt(pow((dest.x - pos.x),2) + pow((dest.y - pos.y),2));
		printf("NAVE %s TRAVELING TO DESTINATION (ETA: %f)\n", tv1.tv_nsec);
		nanosleep(&tv1, &tv2);
		pos.x = dest.x;
		pos.y = dest.y;
		strcpy(posx_str, destx);
		strcpy(posy_str, desty);
		printf("NAVE %s ARRIVED AT %f %f\n", argv[2], pos.x, pos.y);
		break;
	}

	exit(0);
}