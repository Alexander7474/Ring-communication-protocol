#ifndef DRIVER_H
#define DRIVER_H

#include "../common/config.h"

void send_sockg(int sock, char *msg);

void receiv_sockd(int sock, char *msg);

void generate_token(char *buffer);

#endif // !DRIVER_H
