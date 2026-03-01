#include <stdio.h>
#include <stdlib.h>

void FATAL(char * message) {
    perror(message);
    exit(1);
}
