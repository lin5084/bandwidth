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

static unsigned int g_sum = 0;

static void *worker(void *arg)
{
    ThreadArg *t = (ThreadArg*)arg;
    size_t i;
    if(t->type == 0){
        // rd 只读
        volatile unsigned char *p = (volatile unsigned char *)t->buf;
        unsigned int sum = 0;
        for(i=0;i<t->size;i++){
            sum += p[i];
        }
        __sync_fetch_and_add(&g_sum, sum);
    }else if(t->type ==1){
        // wr 只写
        volatile unsigned char *p = (volatile unsigned char *)t->buf;
        for(i=0;i<t->size;i++){
            p[i] = (unsigned char)(i & 0xff);
        }
    }else{
        // cp 拷贝，改用系统memcpy
        char *dst = malloc(t->size);
        memcpy(dst, t->buf, t->size);
        __sync_fetch_and_add(&g_sum, ((volatile unsigned char*)dst)[0]);
        free(dst);
    }
    return NULL;
}

int main(int argc, char** argv)
{
    if(argc !=5){
        printf("Usage: %s rd|wr|cp size_mb threads iterations\n",argv[0]);
        printf("Example: %s cp 64 4 5\n",argv[0]);
        printf("cp=copy, rd=read, wr=write\n");
        printf("size_mb: total memory MB\n");
        printf("threads: thread count\n");
        printf("iterations: run N times and average\n");
        return 1;
    }
    int type;
    if(!strcmp(argv[1],"rd")) type=0;
    else if(!strcmp(argv[1],"wr")) type=1;
    else if(!strcmp(argv[1],"cp")) type=2;
    else {printf("type must be rd wr cp\n");return 1;}

    size_t sz_mb = atoi(argv[2]);
    int th_cnt = atoi(argv[3]);
    int iter = atoi(argv[4]);

    size_t per_thread = sz_mb*1024*1024 / th_cnt;

    pthread_t *ths = malloc(sizeof(pthread_t)*th_cnt);
    ThreadArg *args = malloc(sizeof(ThreadArg)*th_cnt);

    for(int t=0;t<th_cnt;t++){
        args[t].buf = malloc(per_thread);
        args[t].size = per_thread;
        args[t].type = type;
    }

    double total_time = 0.0;
    double total_bw_sum = 0.0;

    printf("Run %d iterations, total %.2f MB per round, %d threads\n", iter, (double)sz_mb, th_cnt);
    for(int round=0; round<iter; round++)
    {
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
        double bw = (double)sz_mb / sec;
        printf("Round[%d] time %.3fs, Bandwidth: %.2f MB/s\n", round+1, sec, bw);
        total_time += sec;
        total_bw_sum += bw;
    }

    printf("===== Average =====\n");
    printf("Avg time: %.3fs, Avg Bandwidth: %.2f MB/s\n", total_time/iter, total_bw_sum/iter);

    for(int t=0;t<th_cnt;t++) free(args[t].buf);
    free(ths);free(args);
    return 0;
}
