#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <errno.h>
#include <tpl.h>
#include "dsort.h"


int main(int argc, char *argv[]){
    int nfiles = argc - 1;
    int a = nfiles / 2;
    int b = nfiles - a;
    pid_t f = fork();
    if(f == -1){
        perror("fork");
        exit(1);
    } else if(f == 0){
        //Children
        if(b == 1){
            //sort
        } else {
            //make babies and merge
        }
    } else {
        //Parent
        //gonna merge
    }
}

numlist_t parse(char * buf, size_t size){
    numlist_t ret;

    return ret;
}

char * to_buf(numlist_t list, size_t *size){
    return NULL;
}

numlist_t merge(numlist_t a, numlist_t b){
    numlist_t ret;
    ret.size = a.count + b.count;
    ret.count = 0;
    ret.list = malloc(ret.size * sizeof(int));
    size_t ap = 0, bp = 0;
    while(ret.count < ret.size){
        if(ap < a.count && bp < b.count){
            if(a.list[ap] >= b.list[bp]){
                ret.list[ret.count++] = a.list[ap++];
            } else {
                ret.list[ret.count++] = b.list[bp++];
            }
        } else if(ap < a.count){
            ret.list[ret.count++] = a.list[ap++];
        } else {
            ret.list[ret.count++] = b.list[bp++];
        }
    }
    return ret;
}
