/* Threads and semaphores in user space.
 */




#include "assert.h"
#include "egos.h"
#include "string.h"

#include "queue.h"
#include "context.h"
//#include "../shared/queue.h"


#define STACKALIGN 0xf
#define STACKSIZE 16 * 1024
/**
 * Thread package implementation
 */

enum state{
	RUNNING,
	READY,
	TERMINATED,
	BLOCKED,
    STOPPED
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

};
struct monitor {
    struct thread *current_t;
    struct thread * idle_t;
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

void thread_init(){
    monitor = malloc(sizeof(struct monitor));
    monitor->thread_ready = malloc(sizeof(struct queue));
    monitor->thread_exited = malloc(sizeof(struct queue));
    monitor->idle_t = malloc(sizeof(struct thread));

    queue_init(monitor->thread_ready);
    queue_init(monitor->thread_exited);
    if(!monitor->thread_ready || !monitor->thread_exited) {
        perror("Allocations for thread queues failed!");
        sys_exit(1);
    }
    monitor->current_t = monitor->idle_t;
    id = 0;


}
void thread_create(void (*f)(void *arg), void *arg,
				   unsigned int stack_size){
    struct thread * t = malloc(sizeof(struct thread));
    id++;

    if(t == NULL){
        perror("Spawning new thread failed");
        return;
    }

    t->ip = f;
   // allocate_stack(&(t->base), &(t->top));
    t->stack = malloc(STACKSIZE);
    t->base = (address_t) &t->stack[STACKSIZE];


    t->arg = arg;
    t->state = READY;
    queue_add(monitor->thread_ready, t);
    printf("thread %d is created\n", id);
    thread_schedule();
}
void ctx_entry(void) {
    /* Invoke the a new thread and call f(), exit()
	 */
    monitor->current_t->state = STOPPED;
    queue_add(monitor->thread_ready, monitor->current_t);

    monitor->current_t = monitor->next_t;
    monitor->current_t->state = RUNNING;
   // printf("@@@@@@@@@@@@@ here ctx_enry!\n");
    (monitor->current_t->ip)(monitor->current_t->arg);
   // printf("Excuted function!\n");
    monitor->current_t->state = TERMINATED;


    queue_add(monitor->thread_exited,monitor->current_t);
    thread_exit();

}
static void thread_schedule(){
   // printf("entering schedule\n");
    struct thread *old = monitor->current_t;
    monitor->next_t = findRunnable();

    if(monitor->next_t == NULL) {
        //TODO: free
        sys_exit(1);
    }
//    if(old == NULL) {
//
//        old = monitor->idle_t;
//        printf("assigned old with idle_t\n");
//    }


    if(monitor->next_t->state == STOPPED) {
        if(old != monitor->next_t) {
            printf("enter stoppped\n");
            ctx_switch(&(old->base), (monitor->next_t->base));
            printf("ctx_switch\n");

        }
    } else if(monitor->next_t->state == READY) {
        if(old != monitor->next_t) {
            ctx_start(&(old->base), (monitor->next_t->base));
           // printf("switch success\n");

        }
    } else {
        printf("??? should not be there\n");

        //Nothing can run because the states are 'BLOCKED'
        return;
    }



}
void thread_yield() {
    //if ready queue is empty
  //  printf("thread yeids\n");
    if(queue_empty(monitor->thread_ready)) return;
  //  printf("ready queue is not empty, setting the current thread stopped\n");
    monitor->current_t->state = STOPPED;
    queue_add(monitor->thread_ready, monitor->current_t);
    thread_schedule();
  //  printf("yields successfully\n");
}
void thread_exit() {
    struct thread * cur;
    while((cur = queue_get(monitor->thread_exited)) != NULL) {
        if(cur->base)
            free(cur->base);
        free(cur);
    }
    if(queue_empty(monitor->thread_ready)) free(monitor->current_t);


}
static struct thread*
findRunnable(){
    struct thread *t1;
    t1 = queue_get(monitor->thread_ready);
    if(t1 == NULL) {
        // ready queue is empty
        return NULL;
    }
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
    struct queue * queue_wait;
};
void sema_init(struct sema *sema, unsigned int count) {
   sema = malloc(sizeof(*sema));
    if(sema == NULL) {
        return;
    }
    queue_init(sema->queue_wait);
    if(sema->queue_wait == NULL) return;
    sema->count = count;


}
void sema_dec(struct sema* sema){

    if(--(sema->count) < 0) {
        monitor->current_t->state = BLOCKED;
        queue_add(sema->queue_wait, monitor->current_t);

    }
    thread_yield();
}
void sema_incr(struct sema* sema) {
    struct thread *thread1;
    if(++(sema->count) <= 0) {
        thread1 = (struct thread *)queue_get(sema->queue_wait);
        thread1->state = READY;
        queue_add(monitor->thread_ready,thread1);
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
void printLine(){
    printf("aaa\n");
}
int main(int argc, char **argv){
    thread_init();
    printf("thread inited\n");
    thread_create(test_code, "thread 1", 16 * 1024);
    //printf("thread 1 is done\n");
    thread_create(test_code, "thread 2", 16 * 1024);
    //printf("thread 2 is done\n");
    test_code("main thread");


    return 0;
}

