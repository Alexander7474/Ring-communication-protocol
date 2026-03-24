/**
 * Taille max d'un message 
 */
#define SMAX 64

/**
 * Taille du caractère urgent (en octet)
*/
#define URGENT_SIZE 1   // 8 bits

/**
 * Taille du token (en octets)
 */
#define TOKEN_SIZE 4    // 32 bits

/**
 * Taille du token (en octets)
 */
#define ADDR_SIZE 4     // 32 bits

/**
 * Taille du contenu (en octets)
 */
#define CONTENT_SIZE 32 // 256 bits, donc 32 octets car 256 / 8 (car 1 octet = 8 bytes) = 32

/**
 * Port utilisé par le protocole
 */
#define PORT 4444

/**
 * Temps maximal d'attente avant de regénérer le token
 */
#define MAX_WAIT 5

/**
 * Chemin où stocker la socket Unix qui est un fichier local
*/
#define UNIX_SOCKET_PATH "/tmp/localsock.sock"
