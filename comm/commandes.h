#ifndef COMMANDES_H
#define COMMANDES_H

#include "../common/config.h"

// Envoyer un message à une machine destination
void emettre(const char * msg, int localsock, const char * destinataire);

// Diffuser un message à toutes les machines de l'anneau
void diffuser(const char * msg, int localsock);

// Récupérer les informations concernant toutes les machines du réseau
void recuperer(int localsock);

#endif