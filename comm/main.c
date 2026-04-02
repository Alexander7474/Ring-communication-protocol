#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "comm.h"
#include "../common/config.h"
#include "../common/error.h"

int main() {

  // Déclaration des variables
  int localsock, conn;
  struct sockaddr_un serv;

  // Structure du serveur
  memset(&serv, 0, sizeof(serv));
  serv.sun_family = AF_UNIX;
  strncpy(serv.sun_path, UNIX_SOCKET_PATH, sizeof(serv.sun_path) - 1);

  localsock = socket(AF_UNIX, SOCK_STREAM, 0);
  if(localsock == -1) FATAL("socket");

  // Création d'une connexion avec le serveur
  conn = connect(localsock, (struct sockaddr *) & serv, sizeof(serv));
  if(conn == -1) FATAL("connect");

  comm(localsock, &serv);
  close(localsock);

  return 0;

};
