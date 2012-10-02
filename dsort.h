#ifndef _DSORT_H_
#define _DSORT_H_

typedef struct numlist {
    int size;
    int count;
    int *list;
} numlist_t;


numlist_t merge(numlist_t a, numlist_t b);


#endif
