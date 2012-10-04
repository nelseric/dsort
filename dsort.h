#ifndef _DSORT_H_
#define _DSORT_H_

typedef struct numlist {
    int size;
    int count;
    int *list;
} numlist_t;

void node(int n, char *files[], FILE * parent);

numlist_t parse(FILE *f);
char * to_buf(numlist_t list, size_t *size);
numlist_t merge(numlist_t a, numlist_t b);


#endif
