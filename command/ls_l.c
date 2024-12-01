#include "stdio.h"
#include "const.h"  
#include "string.h"  
#include "fs.h"
#include "type.h"  
/**
 * @brief ls_l - Print the details of the files in the root directory.
 */  
int main (int args, char* argv[])  
{  
    char* result ;  
  
    result = open_dir_l("/"); 
    // 打印文件头部信息
    printf("NAME    MODE    SIZE    START_SECT   NR_SECTS   DEV    CNT    NUM\n");
    
    printf("%s", result );  
    return 0;  
}  