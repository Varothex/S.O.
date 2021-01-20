#ifndef _ALLOC_PROIECTH
#define _ALLOC_PROIECTH

#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>

typedef char ALIGN[16];
union header {
    struct {
        size_t size;
        unsigned is_free;
        union header *next;
    } s;
    // Deoarece union ia dimensiunea celei mai mari variabile, creeam o variabila de 16 pentru a ne asigura ca si union va fi de 16.
    ALIGN stub;
};
typedef union header header_t;

extern pthread_mutex_t global_malloc_lock;

extern header_t *head, *tail;

extern void *malloc(size_t size);

extern void *realloc(void *block, size_t size);

extern header_t *get_free_block(size_t size);

extern void free(void *block);

extern void print_mem_list();


#endif