#ifndef __WRAPPERS_H
#define __WRAPPERS_H
#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>

#define __NR_START_ELEVATOR 548
#define __NR_ISSUE_REQUEST 549
#define __NR_STOP_ELEVATOR 550

static inline int start_elevator(void) {
    return syscall(__NR_START_ELEVATOR);
}

static inline int sys_issue_request(int start, int dest, int type) {
    return syscall(__NR_ISSUE_REQUEST, start, dest, type);
}

static inline int stop_elevator(void) {
    return syscall(__NR_STOP_ELEVATOR);
}
#endif
