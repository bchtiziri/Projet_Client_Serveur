#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>

#include "buffer/buffer.h"
#include "utils.h"


#define PORT_FREESCORD 4321
#define BUFSIZE 512

/** se connecter au serveur TCP d'adresse donnée en argument sous forme de
 * chaîne de caractère et au port donné en argument
 * retourne le descripteur de fichier de la socket obtenue ou -1 en cas
 * d'erreur. */
int connect_serveur_tcp(char *adresse, uint16_t port);

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <adresse_ip>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int sock = connect_serveur_tcp(argv[1], PORT_FREESCORD);
    if (sock < 0) {
        fprintf(stderr, "Erreur de connexion au serveur\n");
        return EXIT_FAILURE;
    }

    struct buffer *bufsock = buff_create(sock, BUFSIZE);  // buffer sur la socket
    char buffer[BUFSIZE];
    
    //lire message bienvenu
     while (buff_fgets_crlf(bufsock, buffer, sizeof(buffer))) {
     	//crlf_to_lf(buffer);
        if (strcmp(buffer, "\n") == 0) break;
        printf("LU: %s", buffer);  //  debug : ajoute ce printf
    }
    
    // Choix du pseudo
    int ok = 0;
    while (!ok) {
        printf("Entrez votre pseudonyme : ");
        if (!fgets(buffer, BUFSIZE, stdin)) break;
	buffer[strcspn(buffer, "\r\n")] = '\0';  // retirer \n ou \r\n

        char commande[BUFSIZE];
        snprintf(commande, sizeof(commande), "nickname %s", buffer);
        
        buffer[strcspn(buffer, "\r\n")] = '\0';  // sécurité
	/*char *msg = lf_to_crlf(commande); ne marche pas pour l'instant
	printf("[CLIENT] Envoi : %s", commande); // debug
        if (msg) {
            write(sock, msg, strlen(msg));
            free(msg);
        }*/
        strncat(commande, "\r\n", sizeof(commande) - strlen(commande) - 1);
	write(sock, commande, strlen(commande));


        if (buff_fgets_crlf(bufsock, buffer, sizeof(buffer)) == NULL) break;
        
        //test  quelle repnse le serveur envoie 
        printf("Reçu du serveur après nickname : [%s]\n", buffer);


        switch (buffer[0]) {
            case '0': printf("Pseudonyme accepté.\n"); ok = 1; break;
            case '1': printf("Pseudonyme déjà pris.\n"); break;
            case '2': printf("Pseudonyme interdit.\n"); break;
            case '3': printf("Commande invalide.\n"); break;
            default: printf("Réponse inconnue : %s", buffer);
        }
    }
    // Phase de chat
    printf("Vous pouvez maintenant discuter :\n");

    // Préparer poll() sur stdin et socket
    struct pollfd fds[2];
    fds[0].fd = 0 ; 
    fds[0].events = POLLIN;

    fds[1].fd = sock;         
    fds[1].events = POLLIN;

    while (1) {
        int ret = poll(fds, 2, -1);
        if (ret == -1) {
            perror("poll");
            break;
        }

        // Si l'utilisateur a tapé une ligne --> lecture
        if (fds[0].revents & POLLIN) {
            if (fgets(buffer, BUFSIZE, stdin) == NULL) {
                break;
            }
            // Convertir \n -> \r\n avant l'envoi
            buffer[strcspn(buffer, "\r\n")] = '\0'; // retire \n à la fin
            char *msg = lf_to_crlf(buffer);
            if (msg) {
                write(sock, msg, strlen(msg));
                free(msg);
           }
        }

        // Si des données sont arrivées sur la socket
        if (fds[1].revents & POLLIN) {
            /*ancienne version 
            ssize_t n = read(sock, buffer, sizeof(buffer) - 1);
            if (n <= 0) break;
            buffer[n] = '\0';
            printf("[CLIENT] Reçu (raw) : %s", buffer); 
        }*/
            if (buff_fgets_crlf(bufsock, buffer, sizeof(buffer)) == NULL) break;
            	 /*printf("[CLIENT] Reçu : %s", buffer); //  test
   		 fflush(stdout); */   
           	 write(1, buffer, strlen(buffer));
           
        }
       }
    
    buff_free(bufsock);
    printf("Déconnexion.\n");
    close(sock);
    return 0;
}

int connect_serveur_tcp(char *adresse, uint16_t port)
{
    int sock_clt = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_clt < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in s_clt;
    memset(&s_clt, 0, sizeof(s_clt));
    s_clt.sin_family = AF_INET;
    s_clt.sin_port = htons(port);

    if (inet_pton(AF_INET, adresse, &s_clt.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock_clt);
        return -1;
    }

    if (connect(sock_clt, (struct sockaddr *)&s_clt, sizeof(s_clt)) < 0) {
        perror("connect");
        close(sock_clt);
        return -1;
    }

    return sock_clt;
}

