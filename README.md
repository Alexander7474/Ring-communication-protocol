# Token-ring implementation

Ce projet consiste en une implémentaion du protocol Token ring

### Driver

Le driver fonctionne avec plusieurs sockets:   
- sockg et l'envoie de données  
- sockd et la reception de données  
- newsock pour la connection de nouveau client dans l'anneau  

Le driver peut envoyer des données à comm à travers un unix socket quand les données lui sont destiné.

### Comm

Le comm est un interpréteur de commandes qui utilise une socket `localsock` pour parler à son driver et transmettre des paquets dans l'anneau.  
Les commandes utilisables sont :

- help : renvoit la liste des commandes utilisables
- exit : ferme le programme,
- quit : ferme le programme,
- echo [MESSAGE]: envoie un message à toutes les machines de l'anneau,  
- echo [ADRESSE IP] [MESSAGE] : envoie un message à une machine connectée à l'anneau à partir de son adresse IP
- file [ADRESSE IP] [FICHIER] : envoie un fichier à une machine connectée à l'anneau à partir de son adresse IP 
- hosts : renvoie toutes les informations sur les clients de l'anneau        

### Circulation du token 

Le token circule constament dans l'anneau dans un message marqué par un caractère urgent 'f' (free). Si un comm a besoin du token il le récupère et prépare son ou ses message(s) avec un caractère urgent 'u' (used) pour signifier que le token ne peut pas être utilisé. Le comm envoie tous ces message à la machine de destination et le dernier message comporte un autre caractère urgent 'e' (end). Une fois que la machine de reception a recu ce caractère urgent elle relance le token dans l'anneau avec le caractère urgent 'f' (free).

### Communication entre comm et driver

Comm communique avec le driver en utilisant un socket UNIX. Il peut demander le token avec le caractère urgent 'n' (needed). Le driver enregistre sa demande et lui envoie le token dès qu'il recois un message marqué 'f' (free). Le comm fabrique ces messages marqué 'u' et les envoie ensuite ces messages au driver qui les envoies sur l'anneau.

### Connection d'une nouvelle machine

Chaque driver possède un socket prêt à accueillir une nouvelle machine dans l'anneau. Dès qu'un driver recoit une demande sur newsock il connecte le sockg du nouvelle arrivant à sont sockd et envoie un message marqué 'c' (connection) en bout de l'anneau pour que la dernière machine se connect au sockd du nouvelle arrivant.

#### Structure des messages entre driver (en octet)

| Flag| Token | Adresse IP Source | Adresse IP Destination | Contenu 
|:--------  |:--------:     | --------: | :----: | :-----:|
|1|4|4|4|128|

#### Caractère urgent entre comm et driver

| caractère | signification | Usage     |
|:--------  |:--------:     | --------: |
|n|needed | Demande de comm au driver pour le token 

#### Caractère urgent entre driver

| caractère | signification | Usage     |
|:--------  |:--------:     | --------: |
|f | free | Marque les messages avec un token libre  |
|u| used | Marque un message comme utilisé, le driver doit simplement le faire circuler si il ne lui est pas destiné  |
|e| end |  Marque le dernier message d'une communication, le driver doit renvoyer le token en circulation après |
|a| acknowledgement |  Marque la reception d'un message |
|c|connection| Marque une demande de connection pour un driver |
|h|hosts| Marque une demande d'information pour chaque machine de l'anneau après l'utilisation de la commande "hosts" sur un comm |
|s|send filename| Marque l'envoi à la machine de destination du nom de fichier pour que la machine puisse enregistrer le fichier reçu avec le même nom que celui envoyé, le fichier reçu sera stocké dans le répertoire où le programme comm aura été exécuté. |

#### Idée en plus

- Chiffrer l'anneau 
