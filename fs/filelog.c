#include "type.h"
#include "stdio.h"
#include "string.h"
#include "const.h"
#include "protect.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "fs.h"
#include "global.h"

// filelog.c -- write logs into a file
PUBLIC int filelog(char * logstr)  
{  
    const char filename[] = "logs.txt";
    if (first_log==0) {
        log_fd = open(filename, O_CREAT | O_RDWR | O_TRUNC);
        ++first_log;
    }
    
    if (log_fd == -1) {
        printf("Failed to create %s.\n", filename);
        return -1;
    }

    
    int n = write(log_fd, logstr, strlen(logstr));
    // printf("write in fd:%d with %s", fd, logstr);
    assert(n == strlen(logstr));

    // close(fd);
    //printf("filelog:log_fd:%d", log_fd);
    return 0;
} 