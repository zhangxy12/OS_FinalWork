#include "stdio.h"

// 用于显示帮助信息
int main() {
    printf("+---------------------------------------------------------------------+\n");
    printf("| Command Name         | Function Description                         |\n");
    printf("+---------------------------------------------------------------------+\n");
    printf("| help                 | List all available commands.                 |\n");
    printf("| echo <text> <file>   | Write text to a file (create if not exist).  |\n");
    printf("| ps                   | List all processes.                          |\n");
    printf("| kill <name>          | Terminate a process by name.                 |\n");
    printf("| ls                   | List files in current directory.             |\n");
    printf("| ls_l                 | List files with details.                     |\n");
    printf("| touch <file>         | Create an empty file.                        |\n");
    printf("| open_f <file>        | Open/edit file (run if executable).          |\n");
    printf("| rm <file>            | Remove a file.                               |\n");
    printf("| grep <text> <file>   | Search text in a file.                       |\n");
    printf("| set open <log_type>  | Enable a log type (tty, fs, mm, device).     |\n");
    printf("| set close <log_type> | Disable a log type (tty, fs, mm, device).    |\n");
    printf("| command & command    | Run multiple commands concurrently (up to 5).|\n");
    printf("+---------------------------------------------------------------------+\n");

    return 0;
}
