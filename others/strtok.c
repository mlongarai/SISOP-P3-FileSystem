#include <stdio.h>
#include <string.h>


int main(int argc, char** argv){
    char str[] = "/home/user/photos";
    char * pch;

    printf ("Splitting string \"%s\" into tokens:\n",str);

    /*Initialize the strtok() with the string to be tokenized*/
    pch = strtok(str,"/");

    while (pch != NULL){
        printf ("%s\n",pch);

        /* Get the next token. */
        pch = strtok (NULL, "/");
    }

    return 0;
}
