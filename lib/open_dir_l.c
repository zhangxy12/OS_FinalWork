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

// for ls_l  
PUBLIC char* open_dir_l(char* path) {
    static char buf[4000];  // 使用静态缓冲区
    MESSAGE msg;
    msg.type = OPEN_DIR_L;

    // 清空缓冲区
    memset(buf, 0, sizeof(buf));

    // 复制路径到缓冲区
    memcpy(buf, path, strlen(path));

    // 调试输出路径内容
    //printf("open_dir: path = %s\n", path);
    send_recv(BOTH, TASK_FS, &msg);

    // 把目录内容放入静态缓冲区
    memcpy(buf, msg.lBUF, strlen(msg.lBUF));

    return buf;
}
