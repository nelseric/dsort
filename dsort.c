#define _XOPEN_SOURCE 500
#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

#include "dsort.h"

void sigint_handler(int signum){
    if(signum == SIGUSR1){
        exit(1);
    } else if(signum == SIGINT){
        exit(0);
    }
}


int main(int argc, char *argv[]){
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = sigint_handler;
    sigaction(SIGUSR1, &sa, NULL);

    int nfiles = argc - 1;
    int a = nfiles / 2;
    int b = nfiles - a;
    pid_t c1, c2;
    int p_a[2], p_b[2];
    if(pipe(p_a) == 1){
        perror("pipe A");
        exit(1);
    }
    if(pipe(p_b) == -1){
        perror("pipe B");
        exit(1);
    }

    c1 = fork();
    if(c1 == -1){
        perror("fork");
        exit(1);
    } else if(c1 == 0){
        close(p_a[0]);
        FILE * pfile = fdopen(p_a[1], "w");
        node(a, argv + 1, pfile);
        fclose(pfile);
    } else {
        //make another child
        c2 = fork();
        if(c2 == 0){
            close(p_b[0]);
            FILE * pfile = fdopen(p_b[1], "w");
            node(b, argv + a + 1, pfile);
            fclose(pfile);
        } else if(c2 == -1) {
            perror("fork");
            kill(c1, SIGINT);
            exit(1);
        } else {
            close(p_a[1]);
            close(p_b[1]);
            waitpid(c1, NULL, 0);
            waitpid(c2, NULL, 0);

            //Parent
            //gonna merge
        }
    }
}

int int_cmp(const void *a, const void *b){
    return *((const int *) a) - *((const int *)b);
}

void node(int n, char *files[], FILE *parent){
    if(n == 1){
        //Sort
        FILE * f = fopen(files[0], "r");
        numlist_t nl;
        nl.size = 100;
        nl.count = 0;
        nl.list = malloc(nl.size * sizeof(char**));
        if(f != NULL){
            char buf[128];
            while(fgets(buf, 128, f) != NULL){
                if(nl.count < nl.size){
                    nl.list[nl.count++] = atoi(buf);
                } else {
                    nl.size = (nl.size * 3) / 2;
                    nl.list = realloc(nl.list, nl.size * sizeof(char**));
                }
            }
            fclose(f);
            printf("%s - %d\n", files[0], nl.count);
            qsort(nl.list, nl.count, sizeof(int), int_cmp);
            for(int i = 0; i < nl.count; i++){
                fprintf(parent,"%d\n", nl.list[i]);
            }
        } else {
            perror(files[0]);
            kill(0, SIGUSR1);
        }
    } else {
        int na = n/2;
        int nb = n - na;
        pid_t child_a, child_b;
        int p_a[2], p_b[2];
        if(pipe(p_a) == -1){
            perror("pipe");
            kill(0, SIGUSR1);
        }
        if(pipe(p_b) == -1){
            perror("pipe");
            kill(0, SIGUSR1);
        }
        child_a = fork();
        if(child_a == 0){
            close(p_a[0]);
            FILE *f = fdopen(p_a[1], "w");
            node(na, files, f);
            fclose(f);
        } else if(child_a == -1){
            perror("fork child a");
            kill(0, SIGUSR1);
        } else {
            child_b = fork();
            if(child_b == 0){
                close(p_a[0]);
                FILE *f = fdopen(p_a[1], "w");
                node(nb, files + na, f);
                fclose(f);
            } else if(child_a == -1){
            } else if(child_b == -1) {
                perror("fork child b");
            } else {
                close(p_a[1]);
                close(p_b[1]);
                FILE * f1 = fdopen(p_a[0], "r");
                FILE * f2 = fdopen(p_b[0], "r");
                //node parent
                waitpid(child_a, NULL, 0);
                numlist_t alist = parse(f1);
                for(int i = 0; i < alist.count; i++){
                    printf("a - %d\n", alist.list[i]);
                }
                waitpid(child_b, NULL, 0);
                numlist_t blist = parse(f2);
                for(int i = 0; i < blist.count; i++){
                    printf("b - %d\n", blist.list[i]);
                }
                numlist_t m = merge(alist, blist);
                free(alist.list);
                free(blist.list);
                for(int i = 0; i < m.count; i++){
                    printf("merged - %d\n", m.list[i]);
                }
            }
        }
    }
}
numlist_t parse(FILE *f){
    numlist_t nl;
    nl.size = 100;
    nl.count = 0;
    nl.list = malloc(nl.size * sizeof(char**));
    if(f != NULL){
        char buf[128];
        while(fgets(buf, 128, f) != NULL){
            if(nl.count < nl.size){
                nl.list[nl.count++] = atoi(buf);
            } else {
                nl.size = (nl.size * 3) / 2;
                nl.list = realloc(nl.list, nl.size * sizeof(char**));
            }
        }
        fclose(f);
    } else {
        puts("parse stream NULL");
        kill(0, SIGUSR1);
    }

    return nl;
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

