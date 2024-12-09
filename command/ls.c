#include "stdio.h"
#include "const.h"  
#include "string.h"  
#include "fs.h"
#include "type.h"  
  
int main (int args, char* argv[])  
{  
    char* result = 0;  
    result = open_dir("/"); 
    //printf("Returned directory content length: %d\n", strlen(result));
    printf("%s\n", result );  
    return 0;  
}  