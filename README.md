# PressC

## Introduction

Ce projet est une application de messagerie implémentée en langage C. Il permet aux utilisateurs de se connecter à un serveur, d'exécuter une liste de commandes pour interagir et d'envoyer des messages à d'autres utilisateurs connectés.

## Fonctionnalités

- **Connexion au serveur** : Les utilisateurs se connecte au serveur.
- **Envoi de messages** : Permet l'envoi de messages entre utilisateurs.
- **Commandes utilisateur** : Une liste de commandes est disponible pour interagir avec l'application.
- **Gestion des utilisateurs** : Support pour plusieurs utilisateurs connectés simultanément.
- **Affichage des messages** : Affichage des messages reçus en temps réel.

## Arccitecture du projet 

- Une partie client decomposer en plusieurs "Classes"
    - Une partie qui gere les connections
    - Une partie qui s'occupe de gérer les clients au sein du serveur 
    - Une partie ressource qui permet de mutualiser les variables 
    - Un main qui gere l'éxecution du serveur
- Une partie client qui est faites en un one file
- Une partie librairie utils qui permet de mutualiser certaines fonctions fonctions tel que les fonctions d'envoie de message et de reception par exemple

## Installation

### Prérequis

- Un compilateur C (gcc, clang, etc.)
- Un environnement de développement compatible (Linux, macOS, Windows avec Cygwin ou WSL)

### Compilation

Pour compiler le projet, utilisez la commande suivante :

```bash
./compilation.sh
./client.sh (As many times as you want)
