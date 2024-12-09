#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "keyboard.h"
#include "proto.h"
/**
 * @brief kill - Terminate a process by its name.
 * @param argc The number of arguments passed to the program.
 * @param argv The array of arguments, where argv[1] is the process name to kill.
 * @return 0 on success, -1 on failure.
 */
int main(int argc, char* argv[]) {
    // if (argc != 2) {
    //     printf("Usage: kill <process_name>\n");
    //     return -1;  
    // }

    char* target_name = argv[1];  // 获取目标进程名
    MESSAGE msg;
    msg.type = KILL_PROC;  // 请求终止进程

    // 将目标进程的名称传递给系统任务
    msg.BUF = target_name;  // 传递进程名称

    // 发送请求给 TASK_SYS 查找并杀死进程
    send_recv(BOTH, TASK_SYS, &msg);
    
    // 根据返回的结果判断是否成功终止进程
    if (msg.RETVAL == 0) {
        printf("Process '%s' terminated successfully.\n", target_name);
        return 0;
    } else {
        printf("Failed to terminate process '%s'.\n", target_name);
        return -1;
    }
}
