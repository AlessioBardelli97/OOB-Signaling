/**
 * @file threadpool.c
 * @brief Implementazione del threadpool definito nella rispettiva interfaccia.
 *
 * @author Alessio Bardelli 544270
 * 
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */

#include <threadpool.h>

typedef enum {

    immediate_shutdown = 1,
    graceful_shutdown  = 2

} threadpool_shutdown_t;

/**
 *  @struct threadpool_task_t
 */
typedef struct {

    void (*function) (void*);
    void* argument;

} threadpool_task_t;

/**
 *  @struct threadpool_t
 * 
 *  @var notify       Condition variable to notify worker threads.
 *  @var threads      Array containing worker threads ID.
 *  @var thread_count Number of threads
 *  @var queue        Array containing the task queue.
 *  @var queue_size   Size of the task queue.
 *  @var head         Index of the first element.
 *  @var tail         Index of the next element.
 *  @var count        Number of pending tasks
 *  @var shutdown     Flag indicating if the pool is shutting down
 *  @var started      Number of started threads
 */
struct threadpool_t {

    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t* threads;
    threadpool_task_t* queue;
    int thread_count;
    int queue_size;
    int head;
    int tail;
    int count;
    int shutdown;
    int started;
};

/**
 * @function threadpool_thread
 * @brief the worker thread
 * @param threadpool the pool which own the thread
 */
static void* threadpool_thread(void* threadpool) {
    
    threadpool_task_t task;
    threadpool_t* pool = (threadpool_t*)threadpool;

    while (true) {

        pthread_mutex_lock(&(pool->lock));

        while ((pool->count == 0) && (!pool->shutdown))
            pthread_cond_wait(&(pool->notify), &(pool->lock));

        if ((pool->shutdown == immediate_shutdown) || ((pool->shutdown == graceful_shutdown) && (pool->count == 0)))
            break;

        task.function = pool->queue[pool->head].function;
        task.argument = pool->queue[pool->head].argument;
        pool->head = (pool->head + 1) % pool->queue_size;
        pool->count -= 1;

        pthread_mutex_unlock(&(pool->lock));

        (*(task.function))(task.argument);
    }

    pool->started--;

    pthread_mutex_unlock(&(pool->lock)); return NULL;
}

int threadpool_free(threadpool_t* pool) {

    if (!pool || pool->started > 0)
        return -1;

    if (pool->queue)
        free(pool->queue);

    if (pool->threads) {

        free(pool->threads);

        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
    }

    free(pool); return 0;
}

threadpool_t* threadpool_create(int thread_count, int queue_size) {
    
    threadpool_t* pool = NULL; int i;

    if (thread_count <= 0 || thread_count > MAX_THREADS || queue_size <= 0 || queue_size > MAX_QUEUE)
        return NULL;
    
    REALLOC(pool, sizeof(threadpool_t), "threadpool_create: realloc", goto err)

    pool->thread_count = 0;
    pool->queue_size = queue_size;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = pool->started = 0;
    pool->threads = NULL; pool->queue = NULL;

    REALLOC(pool->threads, sizeof(pthread_t) * thread_count, "threadpool_create: realloc 1", goto err)
    REALLOC(pool->queue, sizeof(threadpool_task_t) * queue_size, "threadpool_create: realloc 2", goto err)

    THREAD_ERR(pthread_mutex_init(&(pool->lock), NULL), "threadpool_create: pthread_mutex_init", goto err)
    THREAD_ERR(pthread_cond_init(&(pool->notify), NULL), "threadpool_create: pthread_cond_init", goto err)

    for (i = 0; i < thread_count; i++) {

        THREAD_ERR (
            pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void*)pool),
            "threadpool_create: pthread_create",
            threadpool_destroy(pool, 0); return NULL
        )

        pool->thread_count++;
        pool->started++;
    }

    return pool;

    err: {
     
        if (pool)
            threadpool_free(pool);
        
        return NULL;
    }
}

int threadpool_add(threadpool_t* pool, void (*function) (void*), void* argument) {

    int next = 0;

    if (!pool || !function)
        return -1;

    THREAD_ERR (
        pthread_mutex_lock(&(pool->lock)), 
        "threadpool_add: pthread_mutex_lock", 
        return -1
    )

    next = (pool->tail + 1) % pool->queue_size;

    if (pool->count == pool->queue_size) {
        fprintf(stderr, "threadpool_add: queue full\n");
        pthread_mutex_unlock(&(pool->lock)); return -1; 
    }

    if (pool->shutdown) {
        fprintf(stderr, "threadpool_add: pool shutdown\n");
        pthread_mutex_unlock(&(pool->lock)); return -1; 
    }

    pool->queue[pool->tail].function = function;
    pool->queue[pool->tail].argument = argument;
    pool->tail = next;
    pool->count += 1;

    THREAD_ERR (
        pthread_cond_signal(&(pool->notify)),
        "threadpool_add: signal",
        pthread_mutex_unlock(&(pool->lock)); return -1;
    )

    THREAD_ERR (
        pthread_mutex_unlock(&(pool->lock)),
        "threadpool_add: unlock",
        return -1;
    )
    
    return 0;
}

int threadpool_destroy(threadpool_t* pool, int flags) {

    if (!pool) return -1;

    THREAD_ERR (
        pthread_mutex_lock(&(pool->lock)),
        "threadpool_destroy: pthread_mutex_lock",
        return -1
    )
        
    if (pool->shutdown) {
        fprintf(stderr, "threadpool_destroy: pool already shutdown\n");
        return -1;
    }

    pool->shutdown = (flags & threadpool_graceful) ?
        graceful_shutdown : immediate_shutdown;

    THREAD_ERR (
        pthread_cond_broadcast(&(pool->notify)),
        "threadpool_destroy: broadcast",
        return -1;
    )

    THREAD_ERR (
        pthread_mutex_unlock(&(pool->lock)),
        "threadpool_destroy: pthread_mutex_unlock",
        return -1;
    )
        
    for (int i = 0; i < pool->thread_count; i++) {
        THREAD_ERR (
            pthread_join(pool->threads[i], NULL),
            "threadpool_destroy: pthread_join",
        ) }

    threadpool_free(pool); return 0;
}
