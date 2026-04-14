#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>
#include <ifaddrs.h>

#include "../common/config.h"
#include "../common/error.h"
#include "comm.h"

static bool emission_en_cours = false;  // Permet de savoir si on doit libérer le token ou pas

// Affiche les commandes utilisables via l'interpréteur de commandes 
void help() {

    printf("- help : affiche les commandes utilisables via l'interpréteur de commandes.\n");
    printf("- exit : ferme le programme.\n");
    printf("- quit : ferme le programme.\n");
    printf("- echo [MESSAGE] : envoie un message à toutes les machines connectés à l'anneau si seul le message est fourni.\n");
    printf("- echo [MESSAGE] [ADRESSE IP | HOSTNAME] : envoie un message à une machine connectée à l'anneau à partir de son adresse IP ou de son nom d'hôte.\n");
    printf("- file [FICHIER] [ADRESSE IP | HOSTNAME] : envoie un fichier à une machine connectée à l'anneau à partir de son adresse IP ou de son nom d'hôte.\n");
    printf("- hosts : affiche les informations concernant toutes les machines du réseau.\n");

}

// Retourne l'adresse IP de la machine source pour pouvoir savoir d'où vient le paquet et surtout permettre de s'arrêter à la bonne machine dans le cadre d'une diffusion
static struct in_addr _resoudre_source() {

    struct in_addr addr;
    const char * env_ip = getenv("COMM_IP");

    // Cas où on simule des drivers sur la même machine
    if(env_ip != NULL) {
        if(inet_pton(AF_INET, env_ip, &addr) != 1) FATAL("COMM_IP invalide");
        return addr;
    }

    // Cas où chaque driver est sur une machine indépendante
    struct ifaddrs *ifaddr, *ifa;
    addr.s_addr = 0;

    if(getifaddrs(&ifaddr) == -1) FATAL("getifaddrs");

    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if(ifa->ifa_addr == NULL) continue;
        if(ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, "lo") != 0) {
            struct sockaddr_in * sin = (struct sockaddr_in *) ifa->ifa_addr;
            addr = sin->sin_addr;
            break;
        }
    }

    freeifaddrs(ifaddr);

    if(addr.s_addr == 0) FATAL("Aucune interface réseau trouvée !");
    return addr;

}

// Retourne l'adresse IP du destinataire pour gérer le cas où l'utilisateur renseigne le hostname de la machine destinataire
static struct in_addr _resoudre_destinataire(const char * destinataire) {

    struct in_addr addr;

    // Récupération de l'adresse IP de la machine destinataire si le hostname a été passé en paramètre à la place de l'adresse IP
    // inet_pton retourne 1 si la conversion a réussi, sinon on tente gethostbyname
    if(inet_pton(AF_INET, destinataire, &addr) != 1) {
        struct hostent * hp = gethostbyname(destinataire);
        if(hp == NULL) FATAL("gethostbyname");
        memcpy(&addr, hp->h_addr, hp->h_length);
        // hp->h_addr = hp->h_addr_list[0]
    }

    return addr;

}

// Construit le paquet contenant le caractère urgent, le token, l'adresse IP du destinataire et le contenu
static void _construire_paquet(char * paquet, char urgent, const char * token, struct in_addr source, struct in_addr dest, const char * contenu, int nb_octets) {
    
    int current_octet = 0;

    // Ajoute le caractère urgent au paquet
    paquet[current_octet] = urgent;
    current_octet += URGENT_SIZE;

    // Ajoute le token au paquet
    memcpy(paquet + current_octet, token, TOKEN_SIZE);
    current_octet += TOKEN_SIZE;

    // Ajoute l'adresse IP de la source au paquet
    unsigned long source_long = (unsigned long) source.s_addr;  // Pour matcher avec le driver qui stocke les adresses dans un unsigned long (8 octets)
    memcpy(paquet + current_octet, &source_long, ADDR_SIZE);
    current_octet += ADDR_SIZE;

    // Ajoute l'adresse IP du destinataire au paquet
    unsigned long dest_long = (unsigned long) dest.s_addr;  // Pour matcher avec le driver qui stocke les adresses dans un unsigned long (8 octets)
    memcpy(paquet + current_octet, &dest_long, ADDR_SIZE);
    current_octet += ADDR_SIZE;

    // Ajoute le contenu au paquet
    memset(paquet + current_octet, 0, CONTENT_SIZE);
    memcpy(paquet + current_octet, contenu, nb_octets);

}

// Construit le paquet et l'envoie au Driver pour le faire circuler dans l'anneau jusqu'au destinataire
static void _envoyer_paquets(int localsock, const char * msg, const char * token, const char * destinataire) {

    int cc;
    int msg_len = strlen(msg);
    int nb_octets_envoyes = 0;
    struct in_addr source, addr;

    source = _resoudre_source();
    addr = _resoudre_destinataire(destinataire);

    // Envoyer des paquets tant qu'il reste du contenu à envoyer car il faut gérer le cas où la taille du contenu est supérieur à 256
    while(nb_octets_envoyes < msg_len) {

        int nb_octets_a_envoyer;
        bool dernier_paquet;
        char urgent;
        char paquet[PACKET_SIZE];   // 1 + 4 + 8 + 8 + 32 = 53 octets (taille d'un paquet)

        // Calcul du nombre de bits à envoyer dans ce paquet
        nb_octets_a_envoyer = msg_len - nb_octets_envoyes;
        if(nb_octets_a_envoyer > CONTENT_SIZE) nb_octets_a_envoyer = CONTENT_SIZE;  // S'assure que la partie message du paquet ne dépasse pas 256 bits

        // Détermine si ce paquet sera le dernier envoyé ou pas
        if(nb_octets_envoyes + nb_octets_a_envoyer == msg_len) dernier_paquet = true;
        else dernier_paquet = false;

        // Détermine le caractère urgent à utiliser pour ce paquet
        if(dernier_paquet) urgent = 'e';
        else urgent = 'u';

        // On est en cours d'émission tant qu'on n'a pas envoyé 'e'
        emission_en_cours = !dernier_paquet;

        _construire_paquet(paquet, urgent, token, source, addr, msg + nb_octets_envoyes, nb_octets_a_envoyer);

        // Envoi le paquet
        cc = send(localsock, paquet, PACKET_SIZE, 0);
        if(cc == -1) FATAL("send");
        nb_octets_envoyes += nb_octets_a_envoyer;

    }

}

// Envoi une demande au Driver pour récupérer le Token et le retourne
static void _recuperer_token(int localsock, char * token) {

    int cc;

    // Comm demande le token au Driver en envoyant avec le caractère urgent 'n'
    cc = send(localsock, "n", 1, 0);
    if(cc == -1) FATAL("send");

    // Réception du token envoyé par le Driver
    cc = recv(localsock, token, TOKEN_SIZE, 0);
    if(cc == -1) FATAL("recv");

}

// Demande le token au Driver et le récupère, puis envoie le ou les paquet(s) en fonction de la taille du message
static void _envoyer(int localsock, const char * msg, const char * destinataire) {

    char token[TOKEN_SIZE];

    _recuperer_token(localsock, token);    // Envoi une demande au Driver pour récupérer le Token qui le retourne

    // Fabrication des paquets et les envoie au Driver avec le caractère urgent 'u'
    // Le dernier paquet envoyé aura lui le caractère urgent 'e' pour spécifier la fin de l'émission
    _envoyer_paquets(localsock, msg, token, destinataire);

}

// Envoyer un message à une machine destination
void emettre(const char * msg, int localsock, const char * destinataire) {

    _envoyer(localsock, msg, destinataire);
    printf("Le message %s a bien été envoyé à %s !\n", msg, destinataire);

}

// Diffuser un message à toutes les machines de l'anneau
void diffuser(const char * msg, int localsock) {

    _envoyer(localsock, msg, BROADCAST_ADDR);
    printf("Le message %s a bien été diffusé à toutes les machines connectées à l'anneau !\n", msg);

}

// Permet l'envoie ou la réception d'un fichier (binaire ou ASCII) entre deux machines
void transferer_fichier(const char * fichier, const char * destinataire, int localsock) {

    int cc, nb_octets_lus;
    char token[TOKEN_SIZE], filename[PACKET_SIZE], contenu[CONTENT_SIZE];
    FILE * file;
    struct in_addr source, addr;

    // Ouverture du fichier en mode lecture
    file = fopen(fichier, "rb");    // "Envoit des fichiers binaires ou ASCII"
    if(file == NULL) FATAL("fopen");

    source = _resoudre_source();    // Récupère l'adresse IP de la source
    addr = _resoudre_destinataire(destinataire);    // Récupère l'adresse IP du destinataire pour gérer le cas où l'utilisateur renseigne le hostname de la machine destinataire  

    _recuperer_token(localsock, token);    // Envoi une demande au Driver pour récupérer le Token qui le retourne

    // Envoi d'un paquet contenant le caractère urgent 's' spécifiant le nom du fichier
    _construire_paquet(filename, 's', token, source, addr, fichier, strlen(fichier) + 1);
    cc = send(localsock, filename, PACKET_SIZE, 0);
    if(cc == -1) FATAL("send");

    // Lecture du fichier par tranche de 256 octets 
    while((nb_octets_lus = fread(contenu, 1, CONTENT_SIZE, file)) > 0) {

        bool dernier_paquet;
        char urgent;
        char paquet[PACKET_SIZE];

        // Détermine si ce paquet sera le dernier envoyé ou pas car nous avons atteint la fin du fichier
        if(nb_octets_lus < CONTENT_SIZE) dernier_paquet = true;
        else dernier_paquet = false;

        // Détermine le caractère urgent à utiliser pour ce paquet
        if(dernier_paquet) urgent = 'e';
        else urgent = 'u';

        _construire_paquet(paquet, urgent, token, source, addr, contenu, nb_octets_lus);

        // Envoi le paquet
        cc = send(localsock, paquet, PACKET_SIZE, 0);
        if(cc == -1) FATAL("send");

    }

    fclose(file);
    printf("Le fichier %s a bien été envoyé à %s !\n", fichier, destinataire);

}

// Récupérer les informations concernant toutes les machines du réseau
void recuperer(int localsock) {

    int cc;
    char token[TOKEN_SIZE];
    struct in_addr source, broadcast;

    _recuperer_token(localsock, token);

    // Envoi un paquet contenant le caractère urgent 'h' au Driver pour récupérer les données de toutes les machines connectées à l'anneau
    char paquet[PACKET_SIZE];
    source = _resoudre_source();
    inet_pton(AF_INET, BROADCAST_ADDR, &broadcast);

    _construire_paquet(paquet, 'h', token, source, broadcast, "", 0);

    cc = send(localsock, paquet, PACKET_SIZE, 0);
    if(cc == -1) FATAL("send");

}

// Détermine la commande entrée par l'utilisateur et exécute l'action correspondante
void commande(char * c, int localsock) {

    char * command = strtok(c, " ");

    if(strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) exit(0); // Arrêter le programme via une commande sans utiliser CTRL + C

    else if(strcmp(command, "help") == 0) help();

    else if(strcmp(command, "echo") == 0) {

        // Récupération des paramètres de la commande
        char * msg = strtok(NULL, " ");  // Message à envoyer
        char * destinataire = strtok(NULL, " ");    // Adresse IP ou nom de la machine si l'utilisateur / Sinon broadcast à tout le réseau

        // Vérifie la syntaxe de la commande
        if(msg == NULL) { 
            printf("Usage: echo [MESSAGE] [ADRESSE IP | HOSTNAME]\n"); 
            return;
        }

        if(destinataire != NULL) emettre(msg, localsock, destinataire);    // Envoyer le message à une machine du réseau
        else diffuser(msg, localsock); // Envoyer le message à toutes les machines du réseau

    }

    else if(strcmp(command, "file") == 0) {

        // Récupération des paramètres de la commande
        char * fichier = strtok(NULL, " ");
        char * destinataire = strtok(NULL, " ");

        if(fichier == NULL || destinataire == NULL) { 
            printf("Usage: file [FICHIER] [ADRESSE IP | HOSTNAME]\n"); 
            return; 
        }

        transferer_fichier(fichier, destinataire, localsock);

    }

    else if(strcmp(command, "hosts") == 0) recuperer(localsock);

    else printf("Commande introuvable\n");

}  

// Lit le paquet envoyé et stocke les données importantes à traiter
static void _lire_paquet(int localsock, char * urgent, char ** contenu, struct in_addr * source) {

    static char paquet[PACKET_SIZE];
    int cc;

    // Réception du paquet envoyé envoyé par le Driver au Comm
    cc = recv(localsock, paquet, PACKET_SIZE, 0);
    if(cc == -1) FATAL("recv");

    // Les variables sont directement ajustés
    *urgent = paquet[0];
    *contenu = paquet + URGENT_SIZE + TOKEN_SIZE + ADDR_SIZE + ADDR_SIZE;

    unsigned long source_long;
    memcpy(&source_long, paquet + URGENT_SIZE + TOKEN_SIZE, ADDR_SIZE);
    source->s_addr = (uint32_t) source_long;    // Passage par variable

}

// Mémorise le nom du fichier à recevoir à partir du contenu d'un paquet 's'
static void _recevoir_nom_fichier(const char * contenu, char * filename) {

    strncpy(filename, contenu, CONTENT_SIZE - 1);   // Initialise filename avec le nom du fichier
    filename[CONTENT_SIZE - 1] = '\0';

}

// Écrit un bloc de données dans le fichier en cours de réception
static void _recevoir_bloc_fichier(const char * contenu, char urgent, char * filename) {

    // Création du fichier
    FILE * file = fopen(filename, "ab");
    if(file == NULL) FATAL("fopen");

    fwrite(contenu, 1, CONTENT_SIZE, file);
    fclose(file);

    if(urgent == 'e') {
        printf("Fichier '%s' bien reçu !\n", filename);
        filename[0] = '\0';  // Remise à zéro pour fin de traitement de ce fichier pour laisser la place à une nouvelle réception de fichier
    }

}

// Affiche les informations sur une machine connectée à l'anneau
static void _afficher_informations_machines(const char * contenu) {

    struct in_addr addr;    // Adresse IP de la machine
    char hostname[28];  // Hostname de la machine

    memcpy(&addr, contenu, ADDR_SIZE);  // 8 octets pour l'adresse IP
    memcpy(hostname, contenu + ADDR_SIZE, 28);  // 28 octets pour le hostname
    // Autres informations à récupérer ?
    hostname[27] = '\0';    // Fin des informations récupérées

    printf("\r  %-16s %s\n", inet_ntoa(addr), hostname);  // Affichage propre et régulier

}

// Envoie un ACK au Driver pour confirmer la réception d'un paquet
static void _envoyer_ack(int localsock) {

    int cc = send(localsock, "a", 1, 0);
    if(cc == -1) FATAL("send ack");

}

// Libère le token en envoyant 'f' au Driver
static void _liberer_token(int localsock) {

    int cc = send(localsock, "f", 1, 0);
    if(cc == -1) FATAL("send free");

}

// Réceptionne un paquet envoyé par le Driver au Comm
static int recevoir_paquet(int localsock) {

    char urgent;
    char * contenu;
    struct in_addr source;

    // recevoir_paquet() est une fonction static car elle contient des variables static pour mémoriser le nom du fichier traité entre les appels à recevoir_paquet()
    static char filename[CONTENT_SIZE] = {0};   // Initialise un tableau avec CONTENT_SIZE zéros
    static char message[4096] = {0};    // Initialise un buffer suffisament grand pour reconstituer le message
    static int len = 0;

    _lire_paquet(localsock, &urgent, &contenu, &source);

    if(urgent == 'f') return 0; // Pas d'affichage d'un nouveau prompt comm>

    // Réception du nom du fichier à recevoir dans le premier paquet
    if(urgent == 's') { // send filename
        _recevoir_nom_fichier(contenu, filename);
        _envoyer_ack(localsock);
        return 0; // Traitement terminé dans le cas de la réception du premier paquet pour la réception d'un fichier
    }

    // Réception d'un paquet concernant les informations sur un hôte de l'anneau
    if(urgent == 'i') {
        _afficher_informations_machines(contenu);
        _envoyer_ack(localsock);
        return 0; // Traitement terminé dans le cas de la réception du premier paquet pour la réception d'un fichier
    }

    // Réception d'un ACK = notre émission a bien été reçue
    if(urgent == 'a') {
        if(!emission_en_cours) _liberer_token(localsock);   // On libère le token si on a reçu le 'a' final après notre 'e'
        // Si emission_en_cours == true, on attend encore des acks intermédiaires
        return 0;   // Pas d'affichage d'un nouveau prompt comm>
    }

    // Réception d'un paquet de données pour un message texte ou un bloc de fichier
    if(urgent == 'u' || urgent == 'e') {

        // Détermine si c'est un fichier ou un texte qu'on reçoit dans le paquet de données
        if(filename[0] != '\0') _recevoir_bloc_fichier(contenu, urgent, filename);

        // Cas où c'est un texte
        else {

            // On reconstitue le message texte
            int len_paquet = strnlen(contenu, CONTENT_SIZE);
            if(len + len_paquet < (int)sizeof(message) - 1) {
                memcpy(message + len, contenu, len_paquet);
                len += len_paquet;
            }

            if(urgent == 'e') {
                message[len] = '\0';
                printf("\r%s vous a envoyé un message : %s\ncomm>", inet_ntoa(source), message);
                fflush(stdout);

                // Remise à zéro du buffer pour le prochain message reçu
                len = 0;
                message[0] = '\0';

                _envoyer_ack(localsock);
                return 0;   // Pas d'affichage d'un nouveau prompt comm>
            }

        }

        _envoyer_ack(localsock);
        return 0; // Traitement terminé dans le cas de la réception du premier paquet pour la réception d'un fichier

    }

}

// Menu permettant à l'utilisateur d'utiliser les fonctions
void comm(int localsock) {

    char msg[SMAX];
    fd_set readfds;
    int cc;

    printf("comm> ");
    fflush(stdout); // Nécessaire car sinon le prompt "comm> " n'est pas affiché directement

    while(1) {  // Tant que l'utilisateur ne s'est pas déconnecté = tant que le programme n'a pas été volontairement arrêté

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); // Surveille les commandes entrées par l'utilisateur
        FD_SET(localsock, &readfds);    // Surveille les transmissions envoyées par le Driver au Comm

        // Mise en place du select
        cc = select(localsock + 1, &readfds, NULL, NULL, NULL);
        if(cc == -1) FATAL("select");

        // Traitement d'une commande entrée par l'utilisateur
        if(FD_ISSET(STDIN_FILENO, &readfds)) {
            if(fgets(msg, SMAX, stdin) == NULL) break;  // Stockage de la commande entrée par l'utilisateur
            msg[strcspn(msg, "\n")] = '\0';  // Retire le \n à la fin de la commande
            commande(msg, localsock);    // Détermine la commande entrée par l'utilisateur et exécute l'action correspondante
            printf("comm> ");   // Réaffiche le prompt après réception
            fflush(stdout);
        }

        else if(FD_ISSET(localsock, &readfds)) {

            // Réception d'un paquet envoyé par le Driver au Comm
            if(recevoir_paquet(localsock)) {
                printf("comm> ");   // Réaffiche le prompt après réception
                fflush(stdout);
            }   
            
        }

    }

}