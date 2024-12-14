#include "stdio.h"
#include "string.h"

void shellcode() {
    printf("!!!!!poc success!!!!!\n");
    exit(0);
}

void input() {
    char buf[8] = "1234567";
    char payload[] = {
        0x11, 0x11, 0x11, 0x11,
        0x22, 0x22, 0x22, 0x22,
        0x33, 0x33, 0x33, 0x33, 
        0x44, 0x44, 0x44, 0x44, 
        0x55, 0x55, 0x55, 0x55,
        0x00, 0x10, 0x00, 0x00, 
        0x77, 0x77, 0x77};
    strcpy(buf, payload);
    __asm__ __volatile__("xchg %bx, %bx");
    return;
}

int main(int argc, char** argv) {
    input();
    return 0;
}
