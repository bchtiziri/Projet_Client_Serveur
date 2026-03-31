#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "buffer.h"

/**
 * Programme de test du module buffer :
 * Lit depuis l'entrée standard une ligne à la fois
 * (avec terminator '\n'), en utilisant buff_fgets.
 */
int main(void)
{
	buffer *b = buff_create(STDIN_FILENO, 0);
	if (!b) {
        	perror("buff_create");
        	return EXIT_FAILURE;
    	}

	char ligne[1024];

	printf("Tapez des lignes (Ctrl+D pour quitter) :\n");

	while (buff_fgets(b, ligne, sizeof(ligne))) {
       		 printf("LIGNE LUE : %s", ligne);
    	}
	/*while (buff_fgets_crlf(b, ligne, sizeof(ligne))) {
	 printf("LIGNE LUE : %s", ligne);
    	}*/

   	buff_free(b);
    	printf("\n[FIN] Lecture terminée.\n");
    	return 0;
}

