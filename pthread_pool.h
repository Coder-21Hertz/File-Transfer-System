#ifndef PTHREAD_POOL_H
#define PTHREAD_POOL_H

#include <pthread.h>
#include "tlpi_hdr.h"

void *thread_routine(void *arg);
void pool_init(int max_thread_num);
void *thread_routine(void *arg);
int pool_add_task (void *(*process) (int arg), int arg);
int pool_destroy();

#endif // !PTHREAD_POOL_H