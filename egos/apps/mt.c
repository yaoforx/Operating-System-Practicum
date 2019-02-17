/* Threads and semaphores in user space.
 */


#include "assert.h"
#include "egos.h"
#include "string.h"

#include "queue.h"
#include "context.h"


#define STACKALIGN 0xf
#define STACKSIZE 16 * 1024
#define NSLOTS 3
static struct sema s_empty, s_full, s_lock;
static unsigned int in, out;
static char *slots[NSLOTS];


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
    int id;

};


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

/*
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
  //  monitor->current_t->stack = malloc(STACKSIZE);
 //   monitor->current_t->base = (address_t) & monitor->current_t->stack[STACKSIZE];
 //   // check validity
    if(!monitor->thread_ready || !monitor->thread_exited) {
        perror("Allocations for thread queues failed!");
        sys_exit(1);
    }


}

/*
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
    printf("Adding thread %d to ready queue\n",monitor->current_t->id);
    queue_add(monitor->thread_ready, t);
    thread_schedule();
}


void ctx_entry(void) {
    /* Invoke the a new thread and call f(), exit()
	 */
    monitor->current_t->state = STOPPED;
    printf("1Adding thread %d to ready queue\n",monitor->current_t->id);
    queue_add(monitor->thread_ready, monitor->current_t);

    monitor->current_t = monitor->next_t;
    monitor->current_t->state = RUNNING;

    (monitor->current_t->ip)(monitor->current_t->arg);

    thread_exit();

}


static void thread_schedule(){
  //  printf("entering schedule\n");
    struct thread *old = monitor->current_t;
   // printf("Current running thread is %d\n", monitor->current_t->id);

    monitor->next_t = findRunnable();

    while (old == monitor->next_t) {
        if(old->state == TERMINATED ) {
                monitor->next_t= findRunnable();

        } else return;
    }
    //printf("Thread id %d Found a runnable thread with id %d\n",old->id, monitor->next_t->id);

    if(monitor->next_t->state == READY) {
        ctx_start(&(old->base), (monitor->next_t->base));
    } else if(monitor->next_t->state == STOPPED) {
        if(old->state != BLOCKED && old->state != TERMINATED) {
            old->state = STOPPED;
            queue_add(monitor->thread_ready, old);
        }
        monitor->next_t->state = RUNNING;
        monitor->current_t = monitor->next_t;
        ctx_switch(&(old->base), (monitor->next_t->base));
    } else {
        monitor->next_t->state = RUNNING;
        monitor->current_t = monitor->next_t;
        ctx_switch(&(old->base), (monitor->next_t->base));
    }

}
void thread_yield() {
    if(queue_empty(monitor->thread_ready)) {
        if(monitor->current_t->state == TERMINATED || monitor->current_t->state == BLOCKED) sys_exit(1);
        else return;
    }

    if(monitor->current_t->state != BLOCKED && monitor->current_t->state != TERMINATED) {
        monitor->current_t->state = STOPPED;
        queue_add(monitor->thread_ready, monitor->current_t);
        struct thread * cur;
        while((cur = queue_get(monitor->thread_exited)) != NULL) {
            if(cur->stack)
                free(cur->stack);
            free(cur);
        }
    }
    thread_schedule();

}
void thread_exit() {
    // monitor->current_t->status = TEMINATED;
    // queue_add(monitor->thread_exited, monitor->current_t);
    // thread_yield();

    monitor->current_t->state = TERMINATED;
    queue_add(monitor->thread_exited, monitor->current_t);
    thread_yield();


}
struct thread*
findRunnable(){
    if(queue_empty(monitor->thread_ready)) {
        if (monitor->current_t->state != TERMINATED && monitor->current_t->state != BLOCKED) {
            return monitor->current_t;
        } else {
            sys_exit(1);
        }
    }
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

/*
 * sema_init
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

void sema_dec(struct sema* sema){
 //   printf("Enter Sema_dec with decrement %d\n", sema->count);
    if(sema->count == 0){
       // printf("Enter Sema_dec1\n");
        monitor->current_t->state = BLOCKED;
      //  printf("Enter Sema_dec2\n");
       // printf("current running thread is thread %d\n", monitor->current_t->id);
        queue_add(sema->queue_wait, monitor->current_t);
       // printf("Enter Sema_dec3\n");
        thread_yield();

    } else {
        sema->count--;
    }
   // printf("Hey %d\n", sema->count);

}

void sema_inc(struct sema* sema) {
    struct thread *thread1;
   // printf("Thread %d sema_inc count is %d\n", monitor->current_t->id,sema->count);

    // Yunhao: the if branch should consider the queue, not the count

    if(!queue_empty(sema->queue_wait)) {
        thread1 = (struct thread *)queue_get(sema->queue_wait);

        thread1->state = STOPPED;
        queue_add(monitor->thread_ready,thread1);
    } else {
        sema->count++;
    }

}


static void test_code(void *arg){
    int i;
    for (i = 0; i < 10; i++) {
        printf("%s here: %d\n", arg, i);
        thread_yield();
    }
    printf("%s done\n", arg);
}


static void consumer(void *arg){

    unsigned int i;
    printf("Enter consumer\n");
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
int main(int argc, char **argv){
    // Initialize threading package
    thread_init();

    // Initialize semaphores for producer-consumer
    sema_init(&s_lock, 1);
    sema_init(&s_full, 0);
    sema_init(&s_empty, NSLOTS);
    printf("semaphore initialized\n");
    thread_create(consumer, "consumer 1", 16 * 1024);
    producer("producer 1");

    // TODO: other tests
    return 0;


}

