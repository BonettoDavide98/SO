#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <string.h>
#include "merce.h"

int main (int argc, char * argv[]) {
	struct mesg_buffer {
    	long mesg_type;
    	char mesg_text[100];
	};
	struct mesg_buffer message;
	struct position pos;
	char posx_str[20];
	strcpy(posx_str, argv[3]);
	char posy_str[20];
	strcpy(posy_str, argv[4]);
	sscanf(argv[3], "%lf", &pos.x);
	sscanf(argv[4], "%lf", &pos.y);
	char string_out[100];

	int l = 0;
	while(l < 20) {
		sleep(3);
		message.mesg_type = 1;
		strcpy(string_out, argv[2]);
		strcat(string_out, ",");
		strcat(string_out, posx_str);
		strcat(string_out, ",");
		strcat(string_out, posy_str);
		strcat(string_out, ",");
		strcat(string_out, "0");
		strcpy(message.mesg_text, string_out);
		msgsnd(atoi(argv[6]), &message, (sizeof(long) + sizeof(char) * 100), 0);

		msgrcv(atoi(argv[1]), &message, (sizeof(long) + sizeof(char) * 100), 1, 0);
		printf("NAVE %s RECEIVED : %s\n", argv[2], message.mesg_text);
		l++;
	}

	exit(0);
}