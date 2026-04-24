#ifndef FICHIER_H
#define FICHIER_H

#include "../common/config.h"

// Permet l'envoie ou la réception d'un fichier (binaire ou ASCII) entre deux machines
void transferer_fichier(const char * fichier, const char * destinataire, int localsock);

#endif