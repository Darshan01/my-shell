#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "alstr.h"


//arraylist type to hold multiple strings (char *)

//initialize empty arraylist
void alstr_init(alSTR_t* L, unsigned size){

    if(size < 0){
        fprintf(stderr, "Cannot initialize arraylist with negative size: %i\n", size);
        exit(EXIT_FAILURE);
    }

    //malloc space for the array of strings
    L->data = malloc(size * sizeof(char*));

    if(L->data == NULL){
        fprintf(stderr, "Not enough memory available for malloc in alSTR_init.\n");
        exit(EXIT_FAILURE);
    }

    L->length = 0;
    L->capacity = size;
}

//free all strings inside arraylist, then free the arraylist itself
void alstr_destroy(alSTR_t *L){
    for(int i = 0; i < L->length; i++){
        if(L->data[i] != NULL) free(L->data[i]);
    }
    free(L->data);
}

//push a string into the arraylist
void alstr_push(alSTR_t *L, char *item){
    //if there is no space left, double the capacity with realloc
    if(L->length == L->capacity){
        L->capacity *= 2;
        char** temp = realloc(L->data, L->capacity * sizeof(char *));

        if(temp == NULL){
            fprintf(stderr, "Not enough memory avalable for realloc in alSTR_push.\n");
            exit(EXIT_FAILURE);
        }

        L->data = temp;
    }

    //malloc enough space for the string, then use strcpy to put the string in the arraylist
    if(item == NULL) L->data[L->length] = NULL;
    else{
        L->data[L->length] = malloc(strlen(item) + 1);
        strcpy(L->data[L->length], item);
    }
    
    L->length++;
}

