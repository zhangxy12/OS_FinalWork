#include "type.h"
#include "stdio.h"
#include "string.h"
#include "sys/const.h"
#include "sys/protect.h"
#include "sys/fs.h"
#include "sys/proc.h"
#include "sys/tty.h"
#include "sys/console.h"
#include "sys/global.h"
#include "sys/proto.h"

// 用于计算整数的长度
int int_len(int n) {
    int len = 0;
    if (n == 0) return 1;
    while (n != 0) {
        len++;
        n /= 10;
    }
    return len;
}

int main(int argc, char* argv[]) {
    MESSAGE msg;
    struct proc p;
    printf("PID   NAME      FLAGS\n");
    printf("---   ----      -----\n");
    int i=0;
    for (i = 0; i < NR_TASKS + NR_PROCS; i++) {
        msg.PID = i;
        msg.type = GET_PROC_INFO;
        msg.BUF = &p;
        send_recv(BOTH, TASK_SYS, &msg);
        
        if (p.p_flags != FREE_SLOT) {
            // 打印 PID（6个字符宽度）
            int pid_len = int_len(i);
            int pid_spaces = 6 - pid_len;
            printf("%d", i);
            for (int k = 0; k < pid_spaces; k++) { 
                printf(" ");  // 填充空格
            }

            // 打印进程名（最大10个字符宽度）
            int name_len = strlen(p.name);
            int name_spaces = 10 - name_len;
            printf("%s", p.name);
            for (int k = 0; k < name_spaces; k++) {
                printf(" ");  // 填充空格
            }

            // 打印进程状态
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
  