#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>
#include "merce.h"

struct merce {
    int qty;
    int lifeTime;
};

struct position {
    double x;
    double y;
};