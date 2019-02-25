/* Threads and semaphores in user space.
 */




#include "assert.h"
#include "egos.h"


#include "queue.h"
#include "context.h"
#include "../lib/string.h"


#define STACKSIZE 16 * 1024
#define NSLOTS 3



/**
 * Thread package implementation
 */

enum state{
    READY,
	RUNNING,
    STOPPED,
	TERMINATED,
	BLOCKED,
};


/**
 *  start address of stack : base pointer
 *  top address of stack : top pointer
 *  state of the thread : state
 *  thread id: id
 *  argument : arg pointer
 *  instruction pointer: ip
 *
 */
struct thread{
    address_t base;
    char *stack;
	enum state state;
	void *arg;
	void (*ip)(void *);
    int id; // thread id for debugging purpose

};

/**
 * Monitor: Global struct to keep track of current running thread,
 * ready queue, and exited thread queue
 *
 */
struct monitor {
    struct thread *current_t;
    struct thread * next_t;
    struct queue* thread_ready;
    struct queue* thread_exited;
};


static struct monitor* monitor;
void thread_exit();
void thread_yield();
static void thread_schedule();
void ctx_entry(void);
static struct thread* findRunnable();
static int id;

/**
 *  thread_init initializes the threading package
 */
void thread_init(){
    // Initialize all global data structures for threading
    monitor = malloc(sizeof(struct monitor));
    monitor->thread_ready = malloc(sizeof(struct queue));
    monitor->thread_exited = malloc(sizeof(struct queue));
    monitor->current_t = malloc(sizeof(struct thread));
    id = 0;
    monitor->current_t->id = id;

    queue_init(monitor->thread_ready);
    queue_init(monitor->thread_exited);
    if(!monitor->thread_ready || !monitor->thread_exited) {
        perror("Allocations for thread queues failed!");
        sys_exit(1);
    }


}

/**
 * thread_create creates a thread
 */
void thread_create(void (*f)(void *arg), void *arg,
				   unsigned int stack_size){
    struct thread * t = malloc(sizeof(struct thread));
    id++;
    t->id = id;
    t->ip = f;
    t->stack = malloc(STACKSIZE);
    t->base = (address_t) &t->stack[STACKSIZE];
    t->arg = arg;
    t->state = READY;
    queue_add(monitor->thread_ready, t);
    thread_schedule();
}

/**
 * ctx_entry called by ctx_start to
 * Invoke a new thread and call f(), then exit()
 */
void ctx_entry(void) {
    /**
     * Set current running thread as stopped
     */
    monitor->current_t->state = STOPPED;
    /**
     * Put stopped thread to ready queue
     */
    queue_add(monitor->thread_ready, monitor->current_t);
    monitor->current_t = monitor->next_t;
    monitor->current_t->state = RUNNING;
    /**
     * Invoke f()
     */
    (monitor->current_t->ip)(monitor->current_t->arg);
    /**
     * Exit current thread
     */
    thread_exit();

}

/**
 * Helper function for scheduling threads
 */
static void thread_schedule(){
    struct thread *old = monitor->current_t;

    monitor->next_t = findRunnable();
    /**
     * If current thread is also next runnable thread, return
     */
    if(old == monitor->next_t) return;
    /**
     * Schedule next runnable thread
     */
    if(monitor->next_t->state == READY) {
        /**
         * This thread never got to run before
         */
        ctx_start(&(old->base), (monitor->next_t->base));
    } else if(monitor->next_t->state == STOPPED) {
        /**
         * This thread has already run before
         */
        if(old->state != BLOCKED && old->state != TERMINATED) {
            old->state = STOPPED;
            queue_add(monitor->thread_ready, old);
        }
        monitor->next_t->state = RUNNING;
        monitor->current_t = monitor->next_t;
        ctx_switch(&(old->base), (monitor->next_t->base));
    }

}
void thread_yield() {
    /**
     * If ready queue is empty and current thread is not runnable exit with 1
     */
    if(queue_empty(monitor->thread_ready)) {
        if(monitor->current_t->state == TERMINATED || monitor->current_t->state == BLOCKED) sys_exit(1);
        else return;
    }
    /**
     * Clear the exited queue if current thread is runnable
     */
    if(monitor->current_t->state != BLOCKED && monitor->current_t->state != TERMINATED) {
        struct thread * cur;
        while((cur = queue_get(monitor->thread_exited)) != NULL) {
            if(cur->stack)
                free(cur->stack);
            free(cur);
        }
    }
    /**
     * Schedule for next runnable thread
     */
    thread_schedule();

}
void thread_exit() {
    /**
     * Terminated current running thread, added to exited queue, schedule for next runnable thread
     */
    monitor->current_t->state = TERMINATED;
    queue_add(monitor->thread_exited, monitor->current_t);
    thread_yield();

}
/**
 * Helper function to find next runnable thread from ready queue
 * @return a pointer to a runnable thread
 */
struct thread*
findRunnable(){
    /**
     * ready qyeye is empty means
     * either return current running thread
     * or whole program exits
     */
    if(queue_empty(monitor->thread_ready)) {
        if (monitor->current_t->state != TERMINATED && monitor->current_t->state != BLOCKED) {
            return monitor->current_t;
        } else {
            sys_exit(1);
        }
    }
    /**
     * get next runnable thread from ready queue
     */
    struct thread *t1;
    t1 = queue_get(monitor->thread_ready);

    return t1;
}
/***
 * The end of thread package
 *
 */
/**
 * Semaphores implementations
 *
 */
struct sema {
    int count;
    struct queue* queue_wait;
};

/**
 * sema_init for semaphores packages initialization
 * @param sema
 * @param count
 */
void sema_init(struct sema *sema, unsigned int count) {

    sema->queue_wait = malloc(sizeof(struct queue));
    queue_init(sema->queue_wait);
    if(sema->queue_wait == NULL) {
        free(sema);
        perror("allocation Failed\n");
        return;
    }
    sema->count = count;
}
/**
 * sema_dec takes a pointer to sema, if count is positive, decrement its count
 * otherwise wait for the count to incrementi
 * @param sema
 */

void sema_dec(struct sema* sema){
    if(sema->count == 0){
        monitor->current_t->state = BLOCKED;
        queue_add(sema->queue_wait, monitor->current_t);
        thread_yield();
    } else {
        sema->count--;
    }

}
/**
 * sema_inc takes a pointer to sema, if wait queue is not empty, put a thread to the ready queue
 * otherwise increment its count
 * @param sema
 */
void sema_inc(struct sema* sema) {
    struct thread *thread1;

    if(!queue_empty(sema->queue_wait)) {
        thread1 = (struct thread *)queue_get(sema->queue_wait);
        thread1->state = STOPPED;
        queue_add(monitor->thread_ready,thread1);
    } else {
        sema->count++;
    }

}
/**
 * Test for consumer and producer
 */
static struct sema s_empty, s_full, s_lock;
static unsigned int in, out;
static char *slots[NSLOTS];
static void consumer(void *arg){

    unsigned int i;
    for (i = 0; i < 5; i++) {
// first make sure there’s something in the buffer
        sema_dec(&s_full);

// now grab an entry to the queue
        sema_dec(&s_lock);

        void *x = slots[out++];
        printf("%s: got ’%s’\n", arg, x);
        if (out == NSLOTS) out = 0;
        sema_inc(&s_lock);
// finally, signal producers
        sema_inc(&s_empty);
    }
}


static void producer(void *arg){
    for (;;) {
// first make sure there’s an empty slot.
        sema_dec(&s_empty);
// now add an entry to the queue
        sema_dec(&s_lock);
        slots[in++] = arg;
        if (in == NSLOTS) in = 0;

        sema_inc(&s_lock);
// finally, signal consumers
        sema_inc(&s_full);
    }

}
void test_producer(){
    // Initialize threading package
    thread_init();

    // Initialize semaphores for producer-consumer
    sema_init(&s_lock, 1);
    sema_init(&s_full, 0);
    sema_init(&s_empty, NSLOTS);
    thread_create(consumer, "consumer 1", 16 * 1024);
    producer("producer 1");
}
/**
 * End of consumer and producer test
 */
/**
 * Test for multi-reader locks
 */
static struct sema rcount_mutex, readWrite_lock;
unsigned int rcount, times;
void reader(void *arg){
    /**
     * each reader repeatly reads 10 times
     */
    for(int i = 0; i < times; i++) {
        /**
         * secure the lock protecting rcount
         */
        sema_dec(&rcount_mutex);

        rcount += 1;
        /**
         * if reader is 1, that means this is the very first reader,
         * writer could be still writing, try to acquire writer lock
         *
         */
        if (rcount == 1) {
            sema_dec(&readWrite_lock);
        }
        /**
         * successfully acquired writer lock
         * release the lock protecting rcount
         * and perform reading
         */

        sema_inc(&rcount_mutex);
        printf("%s is successfully reading.\n", arg);
        sema_dec(&rcount_mutex);
        rcount -= 1;
        printf("%s just has finished this reading.\n", arg);
        /**
         * if rcount is 0, no one is reading,
         * release the writer lock
         * so that writer can write
         */
        if (rcount == 0) {
            sema_inc(&readWrite_lock);
        }
        sema_inc(&rcount_mutex);
        /**
         * Humble thread yields each time
         */
        thread_yield();
    }
}


void writer(void *arg){
    for(int i = 0; i < times; i++) {
        /**
         * writer acquires writer lock
         */
        sema_dec(&readWrite_lock);
        /**
         *  successfully got writer lock
         *  perform writing
         */
        printf("%s has gotten a chance to write.\n",arg);
        printf("%s has finished writing.\n", arg);
        /**
         * finished writing, release writer lock
         */
        sema_inc(&readWrite_lock);
        /**
         * Humble thread yields each time
         */
        thread_yield();

    }

}
void test_readWriteLock(){
    thread_init();
    sema_init(&readWrite_lock, 1);
    sema_init(&rcount_mutex, 1);
    rcount = 0, times = 5;
    thread_create(reader, "reader 1", 16 * 1024);
    thread_create(reader, "reader 2", 16 * 1024);
    writer("writer");
}
/**
 * End for multi-reader locks
 */
/**
 * Test for dining philosophers problem
 */
/**
 * 5 forks represented by semaphores
 */
#define number 5
static struct sema forks[number];

void eat_think(void * arg) {
    int num = atoi(arg);
    /**
     * Each philosopher gets to eat [times] times
     */
    for(int i = 0; i < times; i++) {
       /**
        * If this is the last philosopher,
        * change his choice of the first fork
        * to prevent from deadlocks.
        * pick first fork, pick second fork
        * If got both, ready to eat
        * otherwise wait.
        */
        if (num == 4) {
            sema_dec((forks + (num + 1) % 5));
            sema_dec(forks + num);
        } else {
            sema_dec(forks + num);
            sema_dec((forks + (num + 1) % 5));

        }
        printf("%s has got his forks, started to eat...\n", arg);
        printf("%s has finished eating.\n",arg);
        if (num == 4) {
            sema_inc((forks + (num + 1) % 5));
            sema_inc(forks + num);
        } else {
            sema_inc(forks + num);
            sema_inc((forks + (num + 1) % 5));
        }
        /**
         * Humble thread yields each time
         * representing philosopher thinking
         */
        thread_yield();
    }


}
void test_diningPhilosopher(){
    thread_init();
    for(int i = 0; i < 5; i++) {
        sema_init(&forks[i], 1);
    }
    times = 5;

    thread_create(eat_think, "0", 16 * 1024);
    thread_create(eat_think, "1", 16 * 1024);
    thread_create(eat_think, "2", 16 * 1024);
    thread_create(eat_think, "3", 16 * 1024);

    eat_think("4");

}
/**
 * This main function performs three tests
 * Consumer and Producer(should exit with status 1)
 * Multi Reader Locks
 * Dining Philosophers
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char **argv){

    printf("Test multi-reader locks\n");
    test_readWriteLock();
    printf("Test dining philosophers \n");
    test_diningPhilosopher();
    printf("Test consumer and producer\n");
    test_producer();

    return 0;


}

