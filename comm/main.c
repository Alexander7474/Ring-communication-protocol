#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/un.h>

#include "../common/config.h"
#include "../common/error.h"

#include "shell.h"

int main() {

  // Déclaration des variables
  int localsock, conn;
  struct sockaddr_un serv;
  const char * socket_path;

  // Structure du serveur
  memset(&serv, 0, sizeof(serv));
  serv.sun_family = AF_UNIX;

  socket_path = getenv("DRIVER_SOCKET_PATH");
  if(!socket_path) socket_path = UNIX_SOCKET_PATH;

  strncpy(serv.sun_path, socket_path, sizeof(serv.sun_path) - 1);

  localsock = socket(AF_UNIX, SOCK_STREAM, 0);
  if(localsock == -1) FATAL("socket");

  // Création d'une connexion avec le serveur
  conn = connect(localsock, (struct sockaddr *) & serv, sizeof(serv));
  if(conn == -1) FATAL("connect");

  comm(localsock);
  close(localsock);

  return 0;

};
