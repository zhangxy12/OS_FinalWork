#include "type.h"
#include "stdio.h"
#include "string.h"
#include "const.h"
#include "protect.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"

// list process

int main(int argc, char* argv[]) {
    MESSAGE msg;
    struct proc p;
    printf("PID  NAME  FLAGS\n");
    int i=0;
    for (i = 0; i < NR_TASKS + NR_PROCS; i++) {
        msg.PID = i;
        msg.type = GET_PROC_INFO;
        msg.BUF = &p;
        send_recv(BOTH, TASK_SYS, &msg);
        if (p.p_flags != FREE_SLOT) {
            printf("%d    %s    ", i, p.name);
            if (p.p_flags == SENDING) {
                printf("SENDING\n");
            } else if (p.p_flags == RECEIVING) {
                printf("RECEIVING\n");
            } else if (p.p_flags == WAITING) {
                printf("WAITING\n");
            } else if (p.p_flags == HANGING) {
                printf("HANGING\n");
            } else {
                printf("Unknown\n");
            }
        }
    }

}