#include "type.h"
#include "stdio.h"
#include "const.h"
#include "string.h"
#include "fs.h"    
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

int main(int args, char* argv[])  
{  
    char* result;
    char file_name[400];
    
    
    // 打印文件头部信息
    printf("NUM NAME           SIZE              MODE          ST_SECT  NR_SECT  DEV \n");
    printf("--- ----           ----              ----          -------  -------  --- \n");

    // 调用 open_dir 获取当前目录下所有文件名（用空格隔开）
    result = open_dir("/"); 
    // 遍历字符串，手动分割文件名
    int i = 1;
    int j = 0;

    while (result[i] != '\0') {
        if (result[i] == ' ' || result[i] == '\n') {  // 如果遇到空格或换行，分割文件名
            file_name[j] = '\0';  // 结束当前文件名
            struct stat file_stat;
            if (strlen(file_name) > 0 &&strcmp(file_name, " ") != 0 && (stat(file_name, &file_stat) == 0)) {
                // 判断文件类型，并打印相应信息

                char file_type[20];
                if (file_stat.st_mode == 16384) {  // 0x4000 - Directory
                    strcpy(file_type, "I_DIRECTORY");
                } else if (file_stat.st_mode == 32768) {  //0x8000  - Regular file
                    strcpy(file_type, "I_REGULAR");
                } else if (file_stat.st_mode == 8192) {  //  0x2000- Character device
                    strcpy(file_type, "I_CHAR_SPECIAL");
                }else if (file_stat.st_mode == 61440) {  //  I_TYPE_MASK     0170000
                    strcpy(file_type, "I_TYPE_MASK");
                }else if (file_stat.st_mode == 24576) {  //  I_BLOCK_SPECIAL 0060000
                    strcpy(file_type, "I_BLOCK_SPECIAL");
                }else if (file_stat.st_mode == 4096) {  //  I_NAMED_PIPE	0010000
                    strcpy(file_type, "I_NAMED_PIPE");
                } else {
                    strcpy(file_type, "UNKNOWN");  // 如果是其他类型的文件
                }

                // 打印文件的属性
               // 打印 inode 编号
                int num_len = int_len(file_stat.st_ino);
                int num_spaces = 4 - num_len;  // 保证 NUM 占 4 个字符
                printf("%d", file_stat.st_ino);
                for (int k = 0; k < num_spaces; k++) { 
                    printf(" ");  // 手动添加空格填充
                }

                // 打印文件名
                int name_len = strlen(file_name);
                int name_spaces = 15 - name_len;  // 保证 NAME 占 15 个字符
                printf("%s", file_name);
                for (int k = 0; k < name_spaces; k++) {
                    printf(" ");  // 手动添加空格填充
                }

                // 打印文件大小
                int size_len = int_len(file_stat.st_size);
                int size_spaces = 10 - size_len;  // 保证 SIZE 占 10 个字符
                printf("%d bytes", file_stat.st_size);
                for (int k = 0; k < size_spaces; k++) {
                    printf(" ");  // 手动添加空格填充
                }
                

                // 打印文件类型
                int type_len = strlen(file_type);
                int type_spaces = 17 - type_len;  // 保证 MODE 占 17 个字符
                printf("%s", file_type);
                for (int k = 0; k < type_spaces; k++) {
                    printf(" ");  // 手动添加空格填充
                }

                // 打印起始扇区
                int sect_len = int_len(file_stat.st_st_sect);
                int sect_spaces = 8 - sect_len;  // 保证 START_SECT 占 8 个字符
                printf("%d", file_stat.st_st_sect);
                for (int k = 0; k < sect_spaces; k++) {
                    printf(" ");  // 手动添加空格填充
                }

                // 打印占用扇区数
                int nr_sects_len = int_len(file_stat.st_nr_sects);
                int nr_sects_spaces = 9 - nr_sects_len;  // 保证 NR_SECTS 占 9 个字符
                printf("%d", file_stat.st_nr_sects);
                for (int k = 0; k < nr_sects_spaces; k++) {
                    printf(" ");  // 手动添加空格填充
                }

                // 打印设备编号
                int dev_len = int_len(file_stat.st_dev);
                int dev_spaces = 5 - dev_len;  // 保证 DEV 占 5 个字符
                printf("%d", file_stat.st_dev);
                for (int k = 0; k < dev_spaces; k++) {
                    printf(" ");  // 手动添加空格填充
                }

                // 换行
                printf("\n");   
            } 
            j = 0;  
        } else {
            file_name[j++] = result[i]; 
        }
        i++;
    }

    return 0;  
}
