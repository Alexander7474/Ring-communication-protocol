# Token-ring implementation

Ce projet consiste en une implémentaion du protocol Token ring

### Driver

Le driver fonctionne avec plusieurs sockets:   
- sockg et l'envoie de données  
- sockd et la reception de données  
- newsock pour la connection de nouveau client dans l'anneau  

Le driver peut envoyer des données à comm à travers un unix socket quand les données lui sont destiné.

### Comm

Le but sera de faire un programme "user-friendy" dans le style d'un interpréteur de commandes où l'utilisateur enverra les messages / informations qu'il souhaite envoyer, et recevra les messages destinés aux utilisateurs de l'anneau.

- "help" renverra la liste des commandes possibles

	-> echo [MESSAGE] [ADRESSE | HOSTNAME] : envoit un message à toutes les machines de l'anneau ou à un utilisateur spécifique     
	-> file	[FICHIER] [ADRESSE | HOSTNAME] : envoit un fichier à toutes les machines de l'anneau ou à un utilisateur spécifique     
	-> hosts : renvoie toutes les informations sur les clients de l'anneau     
	-> whoami : renvoie les informations qui me sont concernées sur l'anneau     

### Circulation du token 

Le token circule constament dans l'anneau dans un message marqué par un caractère urgent 'f' (free). Si un comm a besoin du token il le récupère et prépare son ou ses message(s) avec un caractère urgent 'u' (used) pour signifier que le token ne peut pas être utilisé. Le comm envoie tous ces message à la machine de destination et le dernier message comporte un autre caractère urgent 'e' (end). Une fois que la machine de reception a recu ce caractère urgent elle relance le token dans l'anneau avec le caractère urgent 'f' (free).

### Communication entre comm et driver

Comm communique avec le driver en utilisant un socket UNIX. Il peut demander le token avec le caractère urgent 'n' (needed). Le driver enregistre sa demande et lui envoie le token dès qu'il recois un message marqué 'f' (free). Le comm fabrique ces messages marqué 'u' et les envoie ensuite ces messages au driver qui les envoies sur l'anneau.

### Connection d'une nouvelle machine

Chaque driver possède un socket prêt à accueillir une nouvelle machine dans l'anneau. Dès qu'un driver recoit une demande sur newsock il connecte le sockg du nouvelle arrivant à sont sockd et envoie un message marqué 'c' (connection) en bout de l'anneau pour que la dernière machine se connect au sockd du nouvelle arrivant.

#### Structure des messages entre driver

| caractère urgent (8 bits)| token (32 bits) | addr source (64 bits) | addr (64 bits) | contenue (256 bits)
|:--------  |:--------:     | --------: | :----: | :-----:|

#### Carcatère urgent entre comm et driver

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
|c|connection| Marque une demande de connection pour un driver 

#### Idée en plus

- Faire des logs
- Chiffré l'anneau 
