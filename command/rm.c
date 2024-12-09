#include "type.h"
#include "stdio.h"

/**
 * @brief rm - remove a file.
 * @param argc The number of arguments passed to the program.
 * @param argv The array of arguments, where argv[1] is the file path to remove.
 * @return 0 on success, -1 on failure.
 */
int main(int argc, char *argv[])
{
    // if (argc != 2) {
    //     printf("Usage: rm <filename>\n");
    //     return -1;  
    // }

    // Attempt to delete the specified file.
    if (unlink(argv[1]) == -1) {
        printf("Error\n");  // Print the error message if unlink fails.
        return -1;        // Return error code if file deletion fails.
    }

    // Inform the user of the successful deletion.
    printf("File %s successfully removed.\n", argv[1]);

    return 0;  // Return success code.
}
