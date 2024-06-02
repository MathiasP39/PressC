# Commandes de Gestion de Fichiers dans l'Application

## Envoi de Fichier depuis le Client

### Fonctionnement Général
À tout moment, un client peut décider d'envoyer un fichier en saisissant la commande associée. Le client liste les fichiers d'un répertoire dédié à l'application (où les fichiers à transférer doivent être préalablement rangés) et demande à l'utilisateur de choisir, dans cette liste, un fichier à envoyer.

### Version 1 (v1)
1. **Commande d'envoi** : Le client envoie la commande « file ».
2. **Sélection du fichier** : Le client sélectionne un fichier à partir de la liste des fichiers du répertoire dédié.
3. **Envoi des métadonnées** : Le client envoie le nom et la taille du fichier au serveur.
4. **Réception sur le serveur** : Le serveur crée un objet pour stocker les données reçues.
5. **Transmission du fichier** : Le client envoie le fichier. Le serveur reçoit les données et les stocke dans l'objet créé.
6. **Clôture** : Une fois la même quantité de données reçue que la taille du fichier, le thread côté serveur clôt le fichier et repasse en mode message.

### Version 2 (v2)
1. **Commande d'envoi** : La commande « file » déclenche un nouveau thread côté client et côté serveur pour gérer l'envoi du fichier.
2. **Communication parallèle** : L'envoi du fichier se fait sur un canal de communication parallèle (nouveau socket, nouvelle connexion, nouveau port sur le serveur pour les messages).

### Amélioration
1. **Affichage des fichiers** : La commande entraîne, côté client, un affichage des différents fichiers du dossier de partage.
2. **Sélection par numéro** : Chaque fichier correspond à un nombre. L'utilisateur peut sélectionner le fichier en entrant le nombre associé.

## Récupération de Fichier depuis le Serveur

### Fonctionnement Général
Un client peut recevoir la liste des fichiers disponibles sur le serveur via une commande particulière.

1. **Demande de liste** : Le client envoie une commande pour obtenir la liste des fichiers disponibles sur le serveur.
2. **Création de socket** : En réponse à cette commande, le serveur crée un nouveau socket et un nouveau thread pour gérer l'envoi du fichier.
3. **Thread dédié** : Le client aura un thread dédié pour récupérer le fichier, permettant ainsi de continuer à échanger des messages textuels en parallèle.

### Processus de Récupération
1. **Commande de récupération** : Le client envoie une commande pour récupérer un fichier spécifique.
2. **Communication parallèle** : Le serveur utilise un canal de communication parallèle pour envoyer le fichier, sans interrompre les canaux de messagerie principaux.

## Conclusion
Ces commandes permettent une gestion efficace des transferts de fichiers dans l'application, en utilisant des threads et des canaux de communication parallèles pour garantir la continuité des échanges de messages textuels tout en envoyant ou recevant des fichiers.
