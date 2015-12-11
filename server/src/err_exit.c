#include "err_exit.h"
#include <stdlib.h>
#include <stdio.h>


// Exit function to handle fatal errors
void err_exit(char* msg)
{
    printf("[Error]: %s \nExiting...\n", msg);
    exit(1);
}
