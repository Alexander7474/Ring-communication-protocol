# Token-ring implementation

Ce projet consiste en une implémentaion du protocol Token ring

### Driver

Le driver fonctionne avec plusieurs sockets:   
- sockg et l'envoie de données  
- sockd et la reception de données  
- newsock pour la connection de nouveau client dans l'anneau  

Le driver peut envoyer des données à comm à travers un unix socket quand les données lui sont destiné.

### Comm



### Circulation du token 

Le token circule constament dans l'anneau dans un packet marqué par un caractère urgent 'f' (free). Si un comm a besoin du token il le récupère et prépare son ou ses packet(s) avec un caractère urgent 'u' (used) pour signifier que le token ne peut pas être utilisé. Le comm envoie tous ces packet à la machine de destination et le dernier packet comporte un autre caractère urgent 'e' (end). Une fois que la machine de reception a recu ce caractère urgent elle relance le token dans l'anneau avec le caractère urgent 'f' (free).

### Communication entre comm et driver

Comm communique avec le driver en utilisant un socket UNIX. Il peut demander le token avec le caractère urgent 'n' (needed). Le driver enregistre sa demande et lui envoie le token dès qu'il recois un packet marqué 'f' (free). Le comm fabrique ces packets marqué 'u' et les envoie ensuite ces packets au driver qui les envoies sur l'anneau.

### Connection d'une nouvelle machine

Chaque driver possède un socket prêt à accueillir une nouvelle machine dans l'anneau. Dès qu'un driver recoit une demande sur newsock il connecte le sockg du nouvelle arrivant à sont sockd et envoie un packet marqué 'c' (connection) en bout de l'anneau pour que la dernière machine se connect au sockd du nouvelle arrivant.

#### Structure des packets entre driver

| caractère urgent (8 bits)| token (32 bits) | addr (32 bits) | contenue (256 bits)
|:--------  |:--------:     | --------: | :----:|

#### Carcatère urgent entre comm et driver

| caractère | signification | Usage     |
|:--------  |:--------:     | --------: |
|n|needed | Demande de comm au driver pour le token 

#### Caractère urgent entre driver

| caractère | signification | Usage     |
|:--------  |:--------:     | --------: |
|f | free | Marque les packets avec un token libre  |
|u| used | Marque un packet comme utilisé, le driver doit simplement le faire circuler si il ne lui est pas destiné  |
|e| end |  Marque le dernier packet d'une communication, le driver doit renvoyer le token en circulation après |
|c|connection| Marque une demande de connection pour un driver 

#### Idée en plus

- Faire des logs
- Chiffré l'anneau 