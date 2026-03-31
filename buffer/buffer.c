#include "buffer.h"

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

struct buffer {
    int fd;
    char data[1024];
    size_t pos; // position actuelle
    size_t len; // nombre de caractères valides
};

buffer *buff_create(int fd, size_t buffsz)
{
    (void)buffsz;

    buffer *b = malloc(sizeof(struct buffer));
    if (!b) {
        return NULL;
    }

    b->fd = fd;
    b->pos = 0;
    b->len = 0;

    return b;
}

int buff_getc(buffer *b)
{
    if (b->pos >= b->len) {
        ssize_t n = read(b->fd, b->data, sizeof(b->data));
        if (n <= 0) {
            return EOF;  // fin de fichier
        }
        b->len = n;
        b->pos = 0;
    }
    return (unsigned char)b->data[b->pos++];
}

int buff_ungetc(buffer *b, int c)
{
    if (b->pos == 0) {
        return -1; // impossible de reculer plus
    }
    b->data[--b->pos] = c;
    return c;
}

void buff_free(buffer *b)
{
    free(b);
}

int buff_eof(const buffer *b)
{
    return b->pos >= b->len;
}

int buff_ready(const buffer *b)
{
    return b->pos < b->len;
}

char *buff_fgets(buffer *b, char *dest, size_t size)
{
    size_t i = 0;
    int c;

    while (i < size - 1) {
        c = buff_getc(b);
        if (c == EOF) break;
        dest[i++] = c;
        if (c == '\n') break;
    }

    dest[i] = '\0';
    return i > 0 ? dest : NULL;
}

char *buff_fgets_crlf(buffer *b, char *dest, size_t size)
{
    size_t i = 0;
    int c;

    while (i < size - 1) {
        c = buff_getc(b);
        if (c == EOF) break;

        if (c == '\r') {
            int next = buff_getc(b);
            if (next == '\n') {
                dest[i++] = '\n';
                break;
            } else {
                buff_ungetc(b, next);
                dest[i++] = '\r';
            }
        } else {
            dest[i++] = c;
            if (c == '\n') break;
        }
    }

    dest[i] = '\0';
    return i > 0 ? dest : NULL;
}

