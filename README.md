# Token-ring implementation

Ce projet consiste en une implémentaion du protocol Token ring

### Driver

Le driver fonctionne avec plusieurs processus fils:   
- Un pour anneausockg et l'envoie de données  
- Un pour anneausockd et la reception de données  
- Un pour un socket d'écoute d'entré d'une nouvelle machine dans l'anneau (TODO)  
   
Le processus de anneausockd peut envoyer les données reçu à anneausockg a travers un pipe.  

Le driver peut envoyer des données à comm à travers un unix socket quand les données lui sont destiné.

### Comm (TODO)

Comm peut demander au driver le token quand il doit faire circuler des données.   