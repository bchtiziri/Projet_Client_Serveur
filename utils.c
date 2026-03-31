#include <string.h>
#include <stdlib.h>

char *crlf_to_lf(char *line_with_crlf)
{
	size_t len = strlen(line_with_crlf);
	char *res = malloc (len + 1);
	if(!res) {
		return NULL;
	}
	char *src = line_with_crlf;
    	char *dst = res;
    	
    	while(*src) {
    		if(*src == '\r' && *(src + 1) == '\n' ) {
    			*dst++ = '\n';
    			src += 2;
    		}else{
    			*dst++ = *src++;
    		}
    	}
    	*dst ='\0';
	return line_with_crlf;
}

char *lf_to_crlf(const char *src) {
    if (!src) return NULL;

    size_t len = strlen(src);
    
    // Alloue suffisamment pour worst case : chaque \n devient \r\n → taille * 2 + 1
    char *dst = malloc(len * 2 + 1);
    if (!dst) return NULL;

    size_t j = 0;
    for (size_t i = 0; i < len; ++i) {
        if (src[i] == '\n') {
            dst[j++] = '\r';
            dst[j++] = '\n';
        } else {
            dst[j++] = src[i];
        }
    }
    dst[j] = '\0';
    return dst;
}
