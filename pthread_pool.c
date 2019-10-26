/*
 * @Description: 线程池
 * @Company: SZU
 * @Author: PerkyRookie
 * @Date: 2019-04-10 09:25:47
 * @LastEditors: PerkyRookie
 * @LastEditTime: 2019-07-19 14:09:30
 */

#include "pthread_pool.h"

typedef struct task
{
    /* 任务需要执行的函数 */
    void *(*process)(int arg);
    /* 执行函数的参数 */
    int arg;
    /* 下一个任务的地址 */
    struct task *next;
}Cthread_task;

/*线程池任务结构*/
typedef struct
{
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_ready;

    /*链表结构，线程池中所有等待任务*/
    Cthread_task *queue_head;
    /*是否销毁线程池*/
    int shutdown;
    /*存放线程id的指针*/
    pthread_t *threadid;
    /*线程池中线程数目*/
    int max_thread_num;
    /*当前等待的任务数*/
    int cur_task_size;

} Cthread_pool;

static Cthread_pool *pool = NULL;
void *thread_routine(void *arg);

/*
 * @description: 初始化线程池
 * @param {int} max_thread_num: 创建几个线程
 * @return: 无
 */
void pool_init(int max_thread_num)
{
    pool = (Cthread_pool *)malloc(sizeof(Cthread_pool));
    /*动态初始化互斥量*/
    pthread_mutex_init(&(pool->queue_lock), NULL);
    /*初始化条件变量*/
    pthread_cond_init(&(pool->queue_ready), NULL);
    //没有任务，头结点为空
    pool->queue_head = NULL;
    //定义最大线程个数
    pool->max_thread_num = max_thread_num;
    //现在任务为0
    pool->cur_task_size = 0;
    //线程池开始工作
    pool->shutdown = 0;
    //申请存放线程池id的数组
    pool->threadid = (pthread_t *)malloc(max_thread_num * sizeof(pthread_t));

    for (int i = 0; i < max_thread_num; i++)
    {
        //创建线程，线程属性为空，参数也设置为空
        if(pthread_create(&(pool->threadid[i]), NULL, thread_routine, NULL) != 0)
            errExit("pool_init.pthread_create");
    }
}

/*
 * @description: 线程运行函数，创建线程时使用，线程运行函数编程遵循如下步骤：
 * @description: 如果当前没有任务，线程被阻塞，等待任务加入唤醒线程，如果有任务加入，线程会被唤醒
 * @param {void *} arg: 传入的参数
 * @return: 无
 */
void *thread_routine(void *arg)
{
    //获取线程id
    printf("starting thread 0x%lx\n", pthread_self());
    while (1)
    {
        //加上互斥锁，动态
        pthread_mutex_lock(&(pool->queue_lock));
        //如果没有任务，则阻塞，等待被唤醒
        while (pool->cur_task_size == 0 && !pool->shutdown)
        {
            printf("thread 0x%lx is waiting\n", pthread_self());
            /*阻塞线程，直到收到条件变量queue_ready通知 */
            pthread_cond_wait(&(pool->queue_ready), &(pool->queue_lock));
        }
        /*线程池销毁时才会运行*/
        if (pool->shutdown)
        {
            /*遇到break,continue,return等跳转语句，千万不要忘记先解锁*/
            pthread_mutex_unlock(&(pool->queue_lock));
            printf("thread 0x%lx will exit\n", pthread_self());
            pthread_exit(NULL);
        }
        /*有任务添加 */
        printf("thread 0x%lx is starting to work\n", pthread_self());

        /*待处理任务减1，并取出链表中的头元素*/
        pool->cur_task_size--;
        Cthread_task *task = pool->queue_head;
        pool->queue_head = task->next;
        /*解锁*/
        pthread_mutex_unlock(&(pool->queue_lock));
        /*调用回调函数（通过pool传入的），执行任务*/
        (*(task->process))(task->arg);
        free(task);
        task = NULL;
    }
    /*这一句应该是不可达的*/
    pthread_exit(NULL);
}

/*
 * @description: 向线程池中加入任务
 * @param {void *} *(process): 线程执行的函数
 * @param {int} arg: 传入执行函数的参数
 * @return: 成功则返回0
 */
int pool_add_task (void *(*process) (int arg), int arg)
{
    /*构造一个新任务*/
    Cthread_task *task = (Cthread_task *)malloc(sizeof(Cthread_task));
    task->process = process;
    task->arg = arg;
    task->next = NULL;

    pthread_mutex_lock(&(pool->queue_lock));
    /*将任务加入到等待队列中*/
    Cthread_task *member = pool->queue_head;
    if (member != NULL)
    {
        while (member->next != NULL)
            member = member->next;
        member->next = task;        //直到结点为空时，加入任务
    }
    else
    {
        pool->queue_head = task;
    }
    //等待任务书+1
    pool->cur_task_size++;
    pthread_mutex_unlock(&(pool->queue_lock));
    //唤醒一个线程
    pthread_cond_signal(&(pool->queue_ready));

    return 0;
}

/*
 * @description: 销毁线程池，等待队列中的任务不会再被执行，但是正在运行的线程会一直把任务运行完后再退出
 * @param {type} 无
 * @return: 成功返回0，错误返回-1
 */
int pool_destroy()
{
    if (pool->shutdown)
        return -1; /*防止两次调用*/
    pool->shutdown = 1;

    /*唤醒所有等待线程，线程池要销毁了*/
    pthread_cond_broadcast(&(pool->queue_ready));

    /*阻塞等待线程退出，否则就成僵尸了*/
    int i;
    for (i = 0; i < pool->max_thread_num; i++)
        pthread_join(pool->threadid[i], NULL);
    free(pool->threadid);

    /*销毁等待队列*/
    Cthread_task *head = NULL;
    while (pool->queue_head != NULL)
    {
        head = pool->queue_head;
        pool->queue_head = pool->queue_head->next;
        free(head);
    }
    /*条件变量和互斥量也别忘了销毁*/
    pthread_mutex_destroy(&(pool->queue_lock));
    pthread_cond_destroy(&(pool->queue_ready));

    free(pool);
    /*销毁后指针置空是个好习惯*/
    pool = NULL;
    return 0;
}