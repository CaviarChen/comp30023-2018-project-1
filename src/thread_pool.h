/* Simple HTTP1.0 Server (Thread Pool)
 * Name: Zijun Chen
 * StudentID: 813190
 * LoginID: zijunc3
 */

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

typedef void (*fun_ptr_t)(int worker_id, void* arg);

void* thread_pool_init(int thread_num);
void thread_pool_add_task(void* _tp, fun_ptr_t fun, void* arg);

#endif
