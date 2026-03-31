#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "user.h"

/** accepter une connection TCP depuis la socket d'écoute sl et retourner un
 * pointeur vers un struct user, dynamiquement alloué et convenablement initialisé */
struct user *user_accept(int sl) {
    struct user *u = malloc(sizeof(struct user));
    if (!u) return NULL;

    u->addr_len = sizeof(u->address);
    u->sock = accept(sl, (struct sockaddr *)&u->address, &u->addr_len);
    if (u->sock < 0) {
        free(u);
        return NULL;
    }
    return u;
}

/** libérer toute la mémoire associée à user */
void user_free(struct user *user) {
    if (!user) return;
    close(user->sock);
    free(user);
}

