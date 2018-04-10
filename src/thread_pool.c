#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "debug_setting.h"
#include "thread_pool.h"


typedef struct task_s {
    fun_ptr_t fun;
    int arg;
    struct task_s *next;
} task_t;

typedef struct {
    int thread_num;
    int task_num;
    task_t *head;
    task_t *tail;

    pthread_mutex_t mutex;
    pthread_cond_t cond;

} thread_pool_t;

typedef struct {
    int worker_id;
    thread_pool_t *tp;
} worker_arg_t;

void* thread_run(void *param);

void* thread_pool_init(int thread_num) {
    thread_pool_t *tp = malloc(sizeof(thread_pool_t));
    if(tp == NULL) return NULL;

    tp->thread_num = thread_num;
    tp->task_num = 0;
    tp->head = NULL;
    tp->tail = NULL;

    pthread_mutex_init(&(tp->mutex), NULL);
    pthread_cond_init(&(tp->cond), NULL);

    pthread_t tid;
    // create threads
    for(int i=0; i<thread_num; i++) {
        worker_arg_t *arg = malloc(sizeof(worker_arg_t));
        arg->tp = tp;
        arg->worker_id = i;
        pthread_create(&tid, NULL, thread_run, arg);

    }
    return tp;
}

void thread_pool_add_task(void* _tp, fun_ptr_t fun, int arg) {
    thread_pool_t *tp = (thread_pool_t*) _tp;

    task_t *t = malloc(sizeof(task_t));

    t->arg = arg;
    t->fun = fun;
    t->next = NULL;

    pthread_mutex_lock(&(tp->mutex));

    if (tp->task_num==0) {
        tp->head = t;
        tp->tail = t;
    } else {
        tp->tail->next = t;
        tp->tail = t;
    }
    tp->task_num += 1;

    pthread_mutex_unlock(&(tp->mutex));
    // notify thread
    pthread_cond_signal(&(tp->cond));
}


void* thread_run(void *param) {
    worker_arg_t *arg = (worker_arg_t*) param;
    int worker_id = arg->worker_id;
    thread_pool_t *tp = arg->tp;
    free(param);
    param = NULL;
    arg = NULL;

    #if DEBUG
        printf("Thread[%d] init.\n", worker_id);
    #endif

    while(1) {
        pthread_mutex_lock(&(tp->mutex));
        // waiting
        pthread_cond_wait(&(tp->cond), &(tp->mutex));

        if(tp->task_num==0) {
            pthread_mutex_unlock(&(tp->mutex));
            continue;
        }
        // get task
        task_t *task = tp->head;
        tp->head = task->next;
        tp->task_num -= 1;
        if (tp->task_num == 0) tp->tail = NULL;

        // unlock and start working
        pthread_mutex_unlock(&(tp->mutex));

        #if DEBUG
            printf("Thread[%d] got task.\n", worker_id);
        #endif

        // execute function
        (task->fun)(worker_id, task->arg);

        free(task);
        task = NULL;
    }

    return NULL;
}
