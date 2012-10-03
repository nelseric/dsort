#ifndef _DSORT_H_
#define _DSORT_H_

typedef struct numlist {
    int size;
    int count;
    int *list;
} numlist_t;


numlist_t parse(char * buf, size_t size);
char * to_buf(numlist_t list, size_t *size);
numlist_t merge(numlist_t a, numlist_t b);


#endif
