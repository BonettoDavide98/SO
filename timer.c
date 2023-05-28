#include <stdio.h>
#include <sys/types.h>
#include <sys/signal.h>

int main (int argc, char * argv[]) {
    int kidpids_shm_id;
    pid_t *kidpids_shm_ptr;
    int n_navi = atoi(argv[2]);
    int n_porti = atoi(argv[3]);

	if((int) (kidpids_shm_id = atoi(argv[1])) < 0) {
		printf("*** shmget error timer ***\n");
		exit(1);
	}
	if((pid_t) (kidpids_shm_ptr = (pid_t) shmat(kidpids_shm_id, NULL, 0)) == -1) {
		printf("*** shmat error porto aval ***\n");
		exit(1);
	}

    sleep(2);
    for(int i = 0; i < 5; i++) {
        sleep(1);
        kill(kidpids_shm_ptr[i], SIGUSR1);
    }

    for(int i = 0; i < (n_navi + n_porti); i++) {
        kill(kidpids_shm_ptr[i], SIGINT);
    }
}