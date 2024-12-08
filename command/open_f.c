#include "stdio.h"
#include "string.h"
#include "const.h"


#define BUFFER_SIZE 256  

// 判断是否为 ELF 文件
int is_elf(char elf_head[16]) {
    if (strncmp((char*)elf_head, "\177ELF", 4) == 0) {
        return 1;  // 是 ELF 文件
    }
    return 0;  // 不是 ELF 文件
}

// 判断文件是否为可执行文件（根据 ELF 文件头）
int is_executable(const char *filename) {
    int fd = open(filename, O_RDWR);  // 打开文件，读写模式
    if (fd < 0) {
        printf("Failed to open file: %s\n", filename);
        return 0;
    }
   
    lseek(fd, 0, SEEK_SET);  

    char elf_head [16];
    int n = read(fd, elf_head, 16);  // 读取 ELF 文件头
    close(fd);

    if (n < 16) {
        
        return 0;  // 读取文件头失败
    }

    // 检查是否为 ELF 文件
    if (is_elf(elf_head) == 1) {
        
        return 1;  // 是 ELF 文件
    }else{
       
        return 0;
    }

}

// 打开文件并读取到内存
int read_file_to_buffer(int fd, char *buffer, int size) {
    int n = read(fd, buffer, size);
    if (n < 0) {
        printf("Error reading file.\n");
        return -1;
    }
    buffer[n] = '\0';  // 确保字符串结束
    return n;
}

// 编辑文件内容：增加、删除
void edit_file(int fd, char *filename) {
    char buf[BUFFER_SIZE];
    char file_content[BUFFER_SIZE * 10];  
    int n;

    // 读取文件内容到缓冲区
    n = read_file_to_buffer(fd, file_content, sizeof(file_content));
    if (n < 0) {
        return;
    }

    // 显示当前文件内容
    printf("Current content of the file:\n");
    write(1, file_content, strlen(file_content));  // 输出到标准输出
    
    while (1) {
        // 提示用户输入命令
        printf("\nEnter command (add <text>, delete <text>, exit): ");
        int flag = 0;   //判断是否非法命令
        // 读取用户命令
        n = read(0, buf, BUFFER_SIZE - 1);  // 从标准输入读取数据，减1是留出空间给'\0'
        if (n < 0) {
            printf("Error reading input.\n");
            break;
        }

        buf[n] = '\0';  // 确保字符串结尾

        // 如果用户输入的是"exit"，就退出编辑
        if (strcmp(buf, "exit") == 0) {
            flag = 1;
            break;
        }
        // 如果用户输入的是"add"，则添加内容
        if (strncmp(buf, "add ", 4) == 0) {
            strcat(file_content, buf + 4);  // 添加去掉"add "的内容
            //strcat(file_content, "\n");  // 添加换行符
            flag = 1;
        }
        // 如果用户输入的是"delete <text>"，则删除匹配的内容
        else if (strncmp(buf, "delete ", 7) == 0) {
            flag = 1;
            char *text_to_delete = buf + 7;  // 获取要删除的文本
            int text_len = strlen(text_to_delete);  // 获取删除文本的长度
            char *start_pos = strstr(file_content, text_to_delete);  // 查找要删除的文本

            // 查找并删除所有匹配的内容
            while (start_pos != NULL) {
                // 找到匹配的文本，将后面的内容前移
                char *move_start = start_pos + text_len;
                while (*move_start != '\0') {
                    *start_pos = *move_start;
                    start_pos++;
                    move_start++;
                }
                *start_pos = '\0';  // 确保字符串结束

                // 继续查找下一个匹配
                start_pos = strstr(start_pos - text_len, text_to_delete);  
            }
        }
        if (flag == 0){
            printf("Error: Invalid command. Please enter a valid command.\n");
            continue;
        }

        fd = open(filename, O_RDWR | O_TRUNC);  // 以读写模式打开文件并清空内容
        
        lseek(fd, 0, SEEK_SET);
        write(fd, file_content, strlen(file_content));  // 直接写入修改后的内容
        

        printf("File content updated:\n%s", file_content);  // 显示修改后的内容
    }

    printf("File edited successfully.\n");
}


int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: open_f <filename>\n");
        return 1;
    }

    const char *filename = argv[1];
    int fd;

    // 判断文件是否是"check"
    if (strcmp(filename, "check") == 0) {
        printf("Cannot be opened. You do not have permission.\n");
        return 0;
    }

    // 判断文件类型
    if (is_executable(filename)) {
        // 如果是可执行文件，则执行该文件
        printf("Running executable: %s\n", filename);
        execl(filename, filename, (char *)NULL);  // 执行文件
        // 如果execl返回，说明执行失败
        printf("Failed to execute %s\n", filename);
        return 1;
    } else {
        // 如果是文本文件，则打开文件并编辑（输出文件内容）
        fd = open(filename, O_RDWR);  // 打开文件用于读写
        if (fd < 0) {
            printf("Failed to open %s\n", filename);
            return 1;
        }
        
        // 调用编辑功能
        edit_file(fd, argv[1]);

        // 关闭文件
        close(fd);
    }

    return 0;
}