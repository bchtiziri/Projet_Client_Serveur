#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>

#include "user.h"
#include "list/list.h"
#include "buffer/buffer.h"
#include "utils.h"

#define PORT 4321
#define BUFFER_SIZE 512

// Définition de types pour compatibilité avec la bibliothèque de listes
typedef struct list list_t;
typedef struct node list_cell_t;

struct list *user_list;
pthread_mutex_t list_mutex;
int pipefd[2]; 

/*Declaration des fonctions*/
int create_listening_sock(uint16_t port);
void *handle_client(void *arg);
void *repeteur_thread(void *arg);

/*Fonction principale*/
int main() {
    // Création du pipe
    if (pipe(pipefd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    // rendre le pipe en lecture non bloquant
    int flags = fcntl(pipefd[0], F_GETFL, 0);
    fcntl(pipefd[0], F_SETFL, flags | O_NONBLOCK);

    // Initialisation du thread qui écoute sur le tube et envoie à tous les users
    pthread_t repeteur;
    if (pthread_create(&repeteur, NULL, repeteur_thread, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }
    pthread_detach(repeteur);
    
    //Créer une liste vide
    user_list = list_create();
    
    //Initialisation du mutex
    if (pthread_mutex_init(&list_mutex, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(EXIT_FAILURE);
    }
    
    //Création de la socket d'écoute
    int listen_sock = create_listening_sock(PORT);
    if (listen_sock < 0) {
        fprintf(stderr, "Erreur : création de la socket d'écoute échouée.\n");
        return EXIT_FAILURE;
    }

    printf("Serveur en écoute sur le port %d...\n", PORT);
    // Se connecter au client
    while (1) {
        struct user *client = user_accept(listen_sock);
        if (!client) {
            perror("user_accept");
            continue;
        }
        
        //Ajout du user à la liste 
        pthread_mutex_lock(&list_mutex);
        list_add_first(user_list, client);
        pthread_mutex_unlock(&list_mutex);

        printf("Connexion acceptée.\n");

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client) != 0) {
            perror("pthread_create");
            user_free(client);
            continue;
        }
        pthread_detach(tid); // Libérer automatiquement les ressources du thread à sa fin
    }
    
    //Détruire le mutex 
    pthread_mutex_destroy(&list_mutex);
    //Fermeture de la socket
    close(listen_sock);
    return 0;
}

/** Créer et configurer une socket d'écoute sur le port donné en argument
 * retourne le descripteur de cette socket, ou -1 en cas d'erreur */
int create_listening_sock(uint16_t port) {
    int sockfd;
    struct sockaddr_in addr;
    int opt = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(sockfd);
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 5) < 0) {
        perror("listen");
        close(sockfd);
        return -1;
    }

    return sockfd;
}

/** Gérer toutes les communications avec le client renseigné dans user */
void *handle_client(void *arg) {
    struct user *client = (struct user *)arg;
    char buffer[BUFFER_SIZE];

    // 1. Envoie du message de bienvenue (en CRLF)
    const char *bienvenue[] = {
        "Bienvenue sur freescord !",
        "Veuillez entrer votre pseudonyme avec : nickname <pseudo>",
        "",
        NULL
    };

    for (int i = 0; bienvenue[i]; i++) {
        char line[BUFFER_SIZE];
        snprintf(line, sizeof(line), "%s\r\n", bienvenue[i]);
        write(client->sock, line, strlen(line));
    }

    // 2. Création du buffer de lecture
    struct buffer *buf = buff_create(client->sock, BUFFER_SIZE);
    if (!buf) {
        perror("buff_create");
        user_free(client);
        return NULL;
    }

    // 3. Boucle de choix du pseudonyme
    int ok = 0;
    while (!ok) {
        if (buff_fgets_crlf(buf, buffer, sizeof(buffer)) == NULL) {
            buff_free(buf);
            user_free(client);
            return NULL;
        }

        if (strncmp(buffer, "nickname ", 9) != 0) {
            dprintf(client->sock, "3 commande invalide\r\n");
            continue;
        }

        char *pseudo = buffer + 9;
        pseudo[strcspn(pseudo, "\r\n")] = '\0';

        if (strlen(pseudo) > 16 || strchr(pseudo, ':')) {
            dprintf(client->sock, "2 pseudo interdit\r\n");
            continue;
        }

        // Vérifier unicé
        pthread_mutex_lock(&list_mutex);
        int taken = 0;
        for (list_cell_t *i = user_list->first; i != NULL; i = i->next) {
            struct user *u = i->elt;
            if (strcmp(u->nickname, pseudo) == 0) {
                taken = 1;
                break;
            }
        }

        if (taken) {
            pthread_mutex_unlock(&list_mutex);
            dprintf(client->sock, "1 pseudo deja utilise\r\n");
            continue;
        }

        // Pseudo accepté
        strncpy(client->nickname, pseudo, 16);
        client->nickname[16] = '\0';
        pthread_mutex_unlock(&list_mutex);
        dprintf(client->sock, "0 pseudonyme accepte\r\n");
        ok = 1;
    }

    // 4. Phase de discussion
    dprintf(client->sock, "Vous pouvez maintenant discuter :\r\n");
    while (1) {
        if (buff_fgets_crlf(buf, buffer, sizeof(buffer)) == NULL)
            break;

        buffer[strcspn(buffer, "\r\n")] = '\0';

        char ligne[BUFFER_SIZE + 32];
        snprintf(ligne, sizeof(ligne), "%s: %s\n", client->nickname, buffer);
        write(pipefd[1], ligne, strlen(ligne));
    }

    // 5. Déconnexion et nettoyage
    printf("Deconnexion de %s\n", client->nickname);
    buff_free(buf);

    pthread_mutex_lock(&list_mutex);
    list_remove_element(user_list, client);
    pthread_mutex_unlock(&list_mutex);

    user_free(client);
    return NULL;
}

/**Fonction du thread repeteur**/
void *repeteur_thread(void *arg) {
    char buffer[1024];

    while (1) {
        ssize_t n = read(pipefd[0], buffer, sizeof(buffer)-1);
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(10000);  // petite pause pour éviter CPU à 100%
                continue;
            } else {
                perror("read pipe");
                continue;
            }
        } else if (n == 0) {
            continue;
        }

        buffer[n] = '\0';
        pthread_mutex_lock(&list_mutex);
        for (list_cell_t *i = user_list->first; i != NULL; i = i->next) {
            struct user *u = (struct user *)i->elt;
            if (write(u->sock, buffer, strlen(buffer)) < 0) {
                perror("write to client");
            }
        }
        pthread_mutex_unlock(&list_mutex);
    }

    return NULL;
}

