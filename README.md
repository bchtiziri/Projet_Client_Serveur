# Freescord — Serveur de chat multi-client en C

Serveur de chat TCP/IP concurrent permettant à plusieurs clients de discuter en temps réel. Chaque client choisit un pseudonyme avant d'accéder au salon commun.

## Compilation
```bash
make        # compile srv et clt
make clean  # supprime les fichiers objets et exécutables
```

## Utilisation
```bash
# Lancer le serveur (port 4321)
./srv

# Connecter un client
./clt <adresse_ip>
# exemple :
./clt 127.0.0.1
```

## Protocole

Le client et le serveur communiquent en texte, chaque ligne terminée par `\r\n`.

**Choix du pseudo :**
```
→  nickname <pseudo>\r\n
←  0 pseudonyme accepte      (succès)
←  1 pseudo deja utilise
←  2 pseudo interdit
←  3 commande invalide
```

**Phase de chat :** chaque message envoyé est diffusé à tous les clients connectés sous la forme `pseudo: message`.

## Architecture

- **serveur** : thread par client + thread répéteur qui lit un pipe et diffuse à tous les connectés
- **client** : `poll()` sur stdin et la socket pour lire/écrire simultanément
- **list** : liste doublement chaînée générique (`void *`) pour gérer les utilisateurs connectés
- **buffer** : lecture bufferisée avec `buff_fgets_crlf` pour reconstituer les messages CRLF fragmentés
- **user** : structure représentant un client connecté (socket, adresse, pseudonyme)

## Structure
```
.
|- serveur.c         # boucle d'acceptation, threads clients, thread répéteur
|- client.c          # connexion, choix du pseudo, boucle poll()
|- user.c / user.h   # accept, free d'un utilisateur
|- utils.c / utils.h # conversion LF ↔ CRLF
|-list/             # liste doublement chaînée générique
|- buffer/           # lecture bufferisée sur fd
|- Makefile
```

