#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "alchar.h"

//initialize empty arraylist
void alch_init(alCH_t *L, int size){

    if(size < 0){
        fprintf(stderr, "Cannot initialize arraylist with negative sizea: %i\n", size);
        exit(EXIT_FAILURE);
    }

    L->data = malloc(size * sizeof(char) + 1);

    if(L->data == NULL){
        fprintf(stderr, "Not enough memory available for malloc in al_init\n");
        exit(EXIT_FAILURE);
    }

    L->pos = 0;
    L->length = 0;
    //start off the arraylist with a null terminator
    //this ensures al_t.data will always will always be a null terminated string
    L->data[0] = '\0';
    L->capacity = size;
}

//free arraylist
void alch_destroy(alCH_t *L){
    free(L->data);
}

//push a character into the arraylist
void alch_push(alCH_t *L, char item){

    //if the length, plus null terminator, is equal to capacity
    //double the capacity with realloc
    if(L->length+1 == L->capacity){
        alch_resize(L);
    }

    //insert the character to the end of the arraylist
    L->data[L->length] = item;
    //put the null terminator after that
    L->data[L->length+1] = '\0';
    L->length++;
}

//pushes a string character by character into the arraylist
void alch_pushStr(alCH_t *L, char *item){

    //loop through characters in char *item
    for(int i = 0; i < strlen(item); i++){
        alch_push(L, item[i]); //push each character with al_push
    }
}

//double arraylist capacity
void alch_resize(alCH_t *L){
    L->capacity *= 2;
    char* temp = (char*) realloc(L->data, L->capacity * sizeof(char));
    
    if(temp == NULL){
        fprintf(stderr, "Not enough memory available for realloc in al_push\n");
        exit(EXIT_FAILURE);

    }

    L->data = temp;
}

//move index at L->length to start
void alch_move(alCH_t *L){
    memmove(L->data, L->data + L->pos + 1, L->length - L->pos - 1);
    L->length -= (L->pos + 1);
    L->pos = 0;
    
}