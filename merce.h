#include <sys/time.h>

struct merce {
    int type;
    int qty;
    struct timeval spoildate;
};

struct position {
    double x;
    double y;
};