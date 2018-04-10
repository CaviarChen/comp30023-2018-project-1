#ifndef THREAD_POOL_H
#define THREAD_POOL_H

typedef void (*fun_ptr_t)(int worker_id, int arg);

void* thread_pool_init(int thread_num);
void thread_pool_add_task(void* _tp, fun_ptr_t fun, int arg);

#endif
