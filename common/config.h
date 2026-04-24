#ifndef CONFIG_H 
#define CONFIG_H

// TODO -- suppremier les doublons et arranger le code qui les utilisent

/**
 * Taille du caractère urgent (en octet)
*/
#define URGENT_SIZE 1   // 8 bits
#define FLAG_SIZE 1 // 8 bits

/**
 * Taille du token (en octets)
*/
#define TOKEN_SIZE 4    // 32 bits
#define TOKEN_OFFSET (FLAG_SIZE)

/**
 * Taille du token (en octets)
*/
#define ADDR_SRC_SIZE 4
#define ADDR_SRC_OFFSET (FLAG_SIZE+TOKEN_SIZE)
#define ADDR_SIZE 4     // 32 bits
#define ADDR_OFFSET (FLAG_SIZE+TOKEN_SIZE+ADDR_SRC_SIZE)

/**
 * Taille du contenu (en octets)
*/
#define CONTENT_SIZE 128
#define DATA_SIZE 128
#define DATA_OFFSET (FLAG_SIZE+TOKEN_SIZE+ADDR_SIZE+ADDR_SRC_SIZE)

/**
 * Taille d'un paquet (en octets)
*/
#define PACKET_SIZE (URGENT_SIZE + TOKEN_SIZE + ADDR_SIZE + ADDR_SIZE + CONTENT_SIZE)   // 1 + 4 + 4 + 4 + 128 = 141 octets (taille d'un paquet)
#define SMAX (FLAG_SIZE + TOKEN_SIZE + ADDR_SIZE + ADDR_SRC_SIZE + CONTENT_SIZE)   // 1 + 4 + 4 + 4 + 32 = 141 octets (taille d'un paquet)

/**
 * Taille des informations d'une machine dans un paquet 'h' (hosts)
 */
#define HOST_NAME_SIZE 16   // 16 octets pour le hostname 
#define HOST_ENTRY_SIZE (ADDR_SIZE + HOST_NAME_SIZE)    // 4 + 16 = 20 octets par machine
#define HOST_MAX_MACHINES (CONTENT_SIZE / HOST_ENTRY_SIZE)  // 128 / 20 (hostname + IP) = 6 machines max

/**
* Taille de la file d'attente d'hosts
 */
#define WAITING_HOST_MAX 32

/* builds "%08X" from TOKEN_SIZE at compile time */
#define TOKEN_FMT "%0" STRINGIFY(TOKEN_SIZE) "X"
#define STRINGIFY(x) STR(x)
#define STR(x) #x

/**
 * Port utilisé par le protocole
*/
#define PORT 4444

/**
 * Adresse de diffusion
*/
#define BROADCAST_ADDR "0.0.0.0"

/**
 * Temps maximal d'attente avant de regénérer le token
*/
#define MAX_WAIT 5

/**
 * Chemin où stocker la socket Unix qui est un fichier local
*/
#define UNIX_SOCKET_PATH "/tmp/localsock.sock"

#endif
