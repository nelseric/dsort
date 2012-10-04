#ifndef _DSORT_H_
#define _DSORT_H_


/**
  Dynamic list data structure, used with malloc and realloc
 */
typedef struct numlist {
    int size;
    int count;
    int *list;
} numlist_t;

void node(int n, char *files[], FILE * parent);

void parse2(numlist_t *na, numlist_t *nb, FILE *fa, FILE *fb);
numlist_t parse(FILE *f);
char * to_buf(numlist_t list, size_t *size);
numlist_t merge(numlist_t a, numlist_t b);


#endif
