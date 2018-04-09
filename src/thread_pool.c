#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "debug_setting.h"

typedef void (*fun_ptr_t)(int worker_id, int arg);

typedef struct task_s {
    fun_ptr_t fun;
    int arg;
    struct task_s *next;
} task_t;

typedef struct thread_pool_s {
    int thread_num;
    int task_num;
    task_t *head;
    task_t *tail;

    pthread_mutex_t mutex;
    pthread_cond_t cond;

} thread_pool_t;


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
        int *arg = malloc(sizeof(*arg));
        *arg = i;
        pthread_create(&tid, NULL, thread_run, arg);

    }
}

void thread_pool_add_task(void* _tp, fun_ptr_t fun, int arg) {
    thread_pool_t *tp = (thread_pool_t*) _tp;

    task_t *t = malloc(sizeof(task_t));

    t->arg = arg;
    t->fun = fun;
    t->next = NULL;

    if (tp->task_num==0) {
        tp->head = t;
        tp->tail = t;
    } else {
        tp->tail->next = t;
        tp->tail = t;
    }
}


void* thread_run(void *param) {
    int worker_id = *((int *) i);
    free(param);

    while(1) {




    }


    return NULL
}
