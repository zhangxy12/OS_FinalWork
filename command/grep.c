#include "type.h"
#include "stdio.h"
#include "const.h"
#include "string.h"
#include "fs.h"

// 查找文件中包含指定文本的行
void grep_file(int fd, char *text) {
    char ch;
    char line[1024];  // 缓冲区存储每行
    int line_index = 0;
    int flag = 0;  

    // 逐字符读取
    while (read(fd, &ch, 1) > 0) {
        if (ch == '\n' ) {
            line[line_index] = '\0';  // 行尾添加终止符

            if (strstr(line, text) != NULL) {
                printf("%s\n", line);
                flag = 1;
            }
            //读取下一行
            line_index = 0;
        } 
        else {
            if (line_index < sizeof(line) - 1) {
                line[line_index++] = ch;
            }
        }
    }
    // 处理最后一行
    if (line_index > 0) {
        line[line_index] = '\0';  
        if (strstr(line, text) != NULL) {
            printf("%s\n", line);
            flag = 1;
        }
    }

    if (!flag) {
        printf("No content you search!\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: grep <text> <filename>\n");
        return 1;
    }

    int fd = open(argv[argc - 1], O_RDWR);
    if (fd < 0) {
        printf("Failed to open %s\n", argv[argc - 1]);
        return 1;
    }

    grep_file(fd, argv[1]);

    close(fd);
    return 0;
}
