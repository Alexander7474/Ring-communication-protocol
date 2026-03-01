#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "driver.h"
#include "../common/config.h"
#include "../common/error.h"

// server qui se parle a lui meme avec son fils (bizarre)
int main (int argc, char *argv[])
{
    if(argc != 2) FATAL("Nombre d'arguments incorrect.\n Usage: ./driver address");
 
    // pipe entre anneausockd et anneausockg
    int ppl[2];
    if (pipe(ppl) < 0) {
        FATAL("pipe");
    }

    pid_t pid;
    pid = fork();
    if(pid == -1){
      FATAL("fork anneausockd");  
    }
    if (pid == 0)
    {
      // Déclaration des variables
      char nom[SMAX]; // On fera plus tard de l'allocation dynamique
      int cc, sock;
      struct sockaddr_in serv;

      close(ppl[0]); // Close read end of pipe

      // "serv." car c'est une structure
      // Structure du serveur
      serv.sin_family = AF_INET;  // On nous demandait d'utiliser le domaine Internet
      serv.sin_port = htons(PORT);    // htons(PORT) pour convertir le numéro de port
      // hp->h_addr = hp->h_addr_list[0]
      serv.sin_addr.s_addr = htonl(INADDR_ANY);

      sock = socket(AF_INET, SOCK_STREAM, 0);  // Création de la socket

      int opt = 1;
      setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
      cc = bind(sock, (struct sockaddr *) &serv, sizeof(serv));
      if(cc == -1) FATAL("bind"); // Erreur à l'attachement

      cc = listen(sock, 5);
      if(cc == -1) FATAL("listen");

      printf("Serveur prêt !\n");
      int lenpserv = sizeof(serv);
      int newsockd = accept(sock, (struct sockaddr *)&serv, (socklen_t *) &lenpserv);

      char msg[128];
      while(1){
        receiv_sockd(newsockd, msg);
        printf("Message recu: %s\n", msg);
        write(ppl[1], msg, sizeof(char)*128);
      }
      close(sock);
      close(ppl[1]);
      exit(0);
    }

    pid = fork();
    if (pid == -1)
    {
      FATAL("fork anneausockg");
    }
    if (pid == 0)
    {
      printf("Ouverture de anneausockg.\n", pid);
      struct hostent * hp;
      struct sockaddr_in serv;
      int sock;

      close(ppl[1]); // Close write end of pipe

      hp = gethostbyname(argv[1]);
      if(hp == NULL) FATAL("gethostbyname");  // Toujours tester pour éviter d'accumuler les erreurs

      // Structure du serveur
      serv.sin_family = AF_INET;
      serv.sin_port = htons(PORT);
      bcopy(hp->h_addr, (char *) & serv.sin_addr, hp->h_length);

      sock = socket(AF_INET, SOCK_STREAM, 0);

      if(connect(sock, (struct sockaddr*) &serv, sizeof(serv)) == -1){
          FATAL("Connect socket");
      }

      char msg[128] = "00000000........";
      char token[TOKEN_SIZE+1];
      token[TOKEN_SIZE] = '\0';

      printf("Client prêt !\n");
      while(1){
        memcpy(token, msg, TOKEN_SIZE); // extraction du token dans le message
        int value = (int)strtol(token, NULL, 16);
        value++;
        snprintf(token, sizeof(token), "%08X", value);  // uppercase, zero-padded to 8 chars
        memcpy(msg, token, TOKEN_SIZE); 
         
        printf("Envoie: %s\n", msg);
        send_sockg(sock, msg);
        read(ppl[0], msg, sizeof(char)*128);
        printf("Message du pipe: %s\n", msg);
        sleep(1);
      }
      close(sock);
      close(ppl[0]);

      exit(0);
    }

    sleep(999);

    return 0;
}
