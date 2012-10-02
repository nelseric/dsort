#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <math.h>
#include <errno.h>

#include "dsort.h"


int main(int argc, char *argv[]){
    int nfiles = argc - 1;
    FILE **files = malloc(sizeof(FILE *) * nfiles);
    for(int i = 0; i < nfiles;i++){
        files[i] = fopen(argv[i], "r");
        if(files[i] == NULL){
            perror("fopen");
            exit(1);
        }
    }

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
