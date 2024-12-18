#include "type.h"
#include "stdio.h"


int main(int argc, char *argv[])  
{  
    // 确保传递了文件名作为命令行参数
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }

    // 调用 open 函数来创建或打开文件
    int fd = open(argv[1], O_CREAT | O_RDWR);  // 使用 O_CREAT 来创建文件，O_RDWR 表示读写模式
    if (fd == -1) {
        printf("Failed to create %s.\n", argv[1]);
        return -1;
    }

    printf("Successfully created %s file \n", argv[1]);


    return 0;
} 