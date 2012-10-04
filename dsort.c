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
#include <sys/select.h>


#include "dsort.h"

/****
  Give me a way to close the program in an error, and handle sigints,
  even though I don't do anything special.
 */
void sigint_handler(int signum){
    if(signum == SIGUSR1){
        exit(1);
    } else if(signum == SIGINT){
        exit(0);
    }
}

/****
  Quick method for printing a numlist
 */
void p(numlist_t n, const char * s, FILE * os){
    for(int i = 0; i < n.count; i++){
        fprintf(os,"%d%s", n.list[i], s);
    }
}

int main(int argc, char *argv[]){
    if(argc < 2){
        fputs("At least one file must be specified\n", stderr);
        exit(1);
    }
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = sigint_handler;
    sigaction(SIGUSR1, &sa, NULL);

    // This splits the files each child has to do in half, and giving 
    // the rounded part to B
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
        fprintf(stderr, "First child A created - %d\n",getpid());
        close(p_a[0]);
        FILE * pfile = fdopen(p_a[1], "w");
        node(a, argv + 1, pfile);
        fclose(pfile);
    } else {
        //make another child
        c2 = fork();
        if(c2 == 0){
            fprintf(stderr, "First child B created - %d\n",getpid());
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
            FILE *f1, *f2;
            f1 = fdopen(p_a[0], "r");
            f2 = fdopen(p_b[0], "r");

            //numlist_t na = parse(f1);
            //numlist_t nb = parse(f2);


            numlist_t na, nb;
            parse2(&na, &nb, f1, f2);


            fprintf(stderr, "%d - Master waiting for all children\n", getpid());
            waitpid(c1, NULL, 0);
            waitpid(c2, NULL, 0);

            fclose(f1);
            fclose(f2);

            fprintf(stderr, "%d - Final Merge\n", getpid());
            numlist_t m = merge(na, nb);

            p(m, "\n",stdout);   
        }
    }
}

//Used for stdlib quicksort
int int_cmp(const void *a, const void *b){
    return *((const int *) a) - *((const int *)b);
}

/*****
  This is the meat of the program. It is the main method of any children in
  the process tree for this program.
  It looks at the number of files it is given, and if 1, it becomes a leaf
  process and sorts the file, piping the sorted list to its parent.
  If n != 1, it becomes a merging parent, and creates children to sort its 
  files.
  Each time a merger is created, it splits its list in half, and makes two
  nodes.
  If a node is given three files, one child will be a leaf, and the other
  child will be a merger.
 */
void node(int n, char *files[], FILE *parent){
    if(n == 1){
        fprintf(stderr, "%d is a leaf\n", getpid());
        //Sort
        FILE * f = fopen(files[0], "r");
        numlist_t nl;
        nl.size = 100;
        nl.count = 0;
        nl.list = malloc(nl.size * sizeof(char**));
        if(f != NULL){
            fprintf(stderr, "%d - %s opened\n", getpid(), files[0]);
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
            fprintf(stderr, "%d - Sorting\n", getpid());
            qsort(nl.list, nl.count, sizeof(int), int_cmp);
            fprintf(stderr, "%d - Done sorting\n", getpid());

            fprintf(stderr, "%d - sending list of size %d to parent\n", getpid(), nl.count);
            for(int i = 0; i < nl.count; i++){
                fprintf(parent,"%d\n", nl.list[i]);
            }
            fprintf(stderr, "%d - Finished sending to parent\n", getpid());
        } else {
            perror(files[0]);
            kill(0, SIGUSR1);
        }
    } else if(n){ //This clause is to catch when only one file is 
        //given to the program, as it would fork bomb otherwise...
        fprintf(stderr, "%d is a merger %d\n", getpid(), n);
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
            fprintf(stderr, "%d created A %d\n", getppid(), getpid());
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
                fprintf(stderr, "%d created B %d\n", getppid(), getpid());
                close(p_b[0]);
                FILE *f = fdopen(p_b[1], "w");
                node(nb, files + na, f);
                fclose(f);
            } else if(child_b == -1) {
                perror("fork child b");
            } else {
                close(p_a[1]);
                close(p_b[1]);
                FILE * f1 = fdopen(p_a[0], "r");
                FILE * f2 = fdopen(p_b[0], "r");
                //node parent
                //numlist_t alist = parse(f1);
                //numlist_t blist = parse(f2);
                numlist_t alist, blist;
                parse2(&alist, &blist, f1, f2);


                fclose(f1);
                fclose(f2);

                fprintf(stderr, "%d - Waiting for %d\n", getpid(), child_b);
                waitpid(child_b, NULL, 0);

                fprintf(stderr, "%d - Waiting for %d\n", getpid(), child_a);
                waitpid(child_a, NULL, 0);

                numlist_t m = merge(alist, blist);

                free(alist.list);
                free(blist.list);


                fprintf(stderr, "%d - sending merged list of size %d to parent\n", getpid(), m.count);
                for(int i = 0; i < m.count; i++){
                    fprintf(parent," %d\n", m.list[i]);
                }
                fprintf(stderr, "%d - finished sending merged list to parent\n", getpid());
            }
        }
    }
}

/****
  This function takes a file pointer, and loads it into a numlist_t
  It used to be used for merge processes, but it doesn't handle
  emptying pipes
 */
numlist_t parse(FILE *f){
    fprintf(stderr, "%d - started parsing file\n", getpid());
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
    } else {
        puts("parse stream NULL");
        kill(0, SIGUSR1);
    }

    fprintf(stderr, "%d - finished parsing file\n", getpid());
    return nl;
}

/*****
  This takes in two file descriptors and builds two numlists, while 
  multiplexing the inputs

  This function is necesary for large files, because with larger files
  we have to clear out both buffers while the other processes are writing
  so they don't fill up
 */
void parse2(numlist_t *na, numlist_t *nb, FILE *fa, FILE *fb){
    int fda = fileno(fa), fdb = fileno(fb);
    int a_status = 1, b_status = 1;

    na->count = nb->count = 0;
    na->size = nb->size = 100;
    na->list = malloc(100 * sizeof(int));
    nb->list = malloc(100 * sizeof(int));

    fprintf(stderr, "%d - Two streams at the same time\n", getpid());
    while(a_status || b_status){
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fda, &fds);
        FD_SET(fdb, &fds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;
        select(fda>fdb?fda+1:fdb+1, &fds, NULL, NULL, &tv);
        char buf[128];
        if(a_status && FD_ISSET(fda, &fds)){  
            if(fgets(buf, 128, fa) == NULL){
                a_status = 0;
            } else {
                if(na->count < na->size){
                    na->list[na->count++] = atoi(buf);
                } else {
                    na->size = na->size * 3 / 2;
                    na->list = realloc(na->list, na->size * sizeof(int));
                }
            }
        }
        if(b_status && FD_ISSET(fdb, &fds)){
            if(fgets(buf, 128, fb) == NULL){
                b_status = 0;
            } else {
                if(nb->count < nb->size){
                    nb->list[nb->count++] = atoi(buf);
                } else {
                    nb->size = nb->size * 3 / 2;
                    nb->list = realloc(nb->list, nb->size * sizeof(int));
                }
            }
        }
    }
    fprintf(stderr, "%d - Finished reading children\n", getpid());
}


/***
  Takes two numlists and merges them togather by creating a new numlist
  and going theough the sorted lists and picking the smallest head off
  each list pointed to by thee cursor.
 */
numlist_t merge(numlist_t a, numlist_t b){
    fprintf(stderr, "%d - starting merge\n", getpid());
    numlist_t ret;
    ret.size = a.count + b.count;
    ret.count = 0;
    ret.list = malloc(ret.size * sizeof(int));
    size_t ap = 0, bp = 0;
    while(ret.count < ret.size){
        if(ap < a.count && bp < b.count){
            if(a.list[ap] <= b.list[bp]){
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
    fprintf(stderr, "%d - finished merging\n", getpid());
    return ret;
}

