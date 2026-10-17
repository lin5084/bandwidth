#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

typedef struct {
    char *buf;
    size_t size;
    int type; //0 rd,1 wr,2 cp
} ThreadArg;

#define BLOCK (1024*1024)
static void *worker(void *arg)
{
    ThreadArg *t = (ThreadArg*)arg;
    size_t i;
    if(t->type == 0){
        unsigned sum=0;
        for(i=0;i<t->size;i++) sum += t->buf[i];
        (void)sum;
    }else if(t->type ==1){
        for(i=0;i<t->size;i++) t->buf[i] = (char)(i&0xff);
    }else{
        char *dst = malloc(t->size);
        for(i=0;i<t->size;i++) dst[i] = t->buf[i];
        free(dst);
    }
    return NULL;
}

int main(int argc, char** argv)
{
    if(argc !=4){
        printf("Usage: %s rd|wr|cp size_mb threads\n",argv[0]);
        printf("Example: %s cp 64 4\n",argv[0]);
        return 1;
    }
    int type;
    if(!strcmp(argv[1],"rd")) type=0;
    else if(!strcmp(argv[1],"wr")) type=1;
    else if(!strcmp(argv[1],"cp")) type=2;
    else {printf("type must be rd wr cp\n");return 1;}

    size_t sz_mb = atoi(argv[2]);
    int th_cnt = atoi(argv[3]);
    size_t per_thread = sz_mb*1024*1024 / th_cnt;

    pthread_t *ths = malloc(sizeof(pthread_t)*th_cnt);
    ThreadArg *args = malloc(sizeof(ThreadArg)*th_cnt);

    for(int t=0;t<th_cnt;t++){
        args[t].buf = malloc(per_thread);
        args[t].size = per_thread;
        args[t].type = type;
    }

    struct timespec t0,t1;
    clock_gettime(CLOCK_MONOTONIC,&t0);
    for(int t=0;t<th_cnt;t++){
        pthread_create(&ths[t],NULL,worker,&args[t]);
    }
    for(int t=0;t<th_cnt;t++){
        pthread_join(ths[t],NULL);
    }
    clock_gettime(CLOCK_MONOTONIC,&t1);

    double sec = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec)/1e9;
    double total_mb = (double)sz_mb;
    double bw = total_mb / sec;
    printf("Total %.2f MB, time %.3fs, Bandwidth: %.2f MB/s\n",total_mb,sec,bw);

    for(int t=0;t<th_cnt;t++) free(args[t].buf);
    free(ths);free(args);
    return 0;
}
