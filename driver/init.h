#ifndef INIT_H
#define INIT_H

int init_inet_listenner(struct sockaddr_in *servd);
int init_unix_listenner(struct sockaddr_un *servcomm);

#endif // ! INIT_H
