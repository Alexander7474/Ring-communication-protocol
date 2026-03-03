# Token-ring implementation

Ce projet consiste en une implémentaion du protocol Token ring

### Driver

Le driver fonctionne avec plusieurs sockets:   
- sockg et l'envoie de données  
- newsockd et la reception de données
- sockd pour la connection de nouveau client dans l'anneau

Le driver peut envoyer des données à comm à travers un unix socket quand les données lui sont destiné.

### Comm (TODO)

Comm peut demander au driver le token quand il doit faire circuler des données.   
