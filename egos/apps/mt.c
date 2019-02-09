/* Threads and semaphores in user space.
 */


#include "assert.h"
#include "egos.h"
#include "string.h"
#include "queue.h"
#include "context.h"
//#include "../shared/queue.h"

#define STACKALIGN 0xf
/**
 * Thread package implementation
 */
enum state{
	RUNNING,
	READY,
	TERMINATED,
	BLOCKED,
    STOPPED,
} state;
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
	void *base;
	void *top;
	enum state state;
	void *arg;
	void (*ip)(void *);

};
struct monitor {
    struct thread *current_t;

    struct queue* thread_ready;

    struct queue* thread_exited;
};
static struct monitor monitor;
void thread_exit();
void thread_yield();
static void thread_schedule();
void ctx_entry(void);
struct thread* findRunnable();
void thread_init(){
    monitor.thread_ready = malloc(sizeof(struct queue *));
    monitor.thread_exited = malloc(sizeof(struct queue *));
    queue_init(monitor.thread_ready);
    queue_init(monitor.thread_exited);
    if(!monitor.thread_ready || !monitor.thread_exited) {
        perror("Allocations for thread queues failed!");
        sys_exit(1);
    }
}
void thread_create(void (*f)(void *arg), void *arg,
				   unsigned int stack_size){
    struct thread * t = malloc(sizeof(struct thread));
    if(t == NULL){
        perror("Spawning new thread failed");
        return;
    }
    printf("inside thread 1 malloc\n");
    t->ip = f;

    if((t->base = malloc(stack_size) == NULL)){
        perror("Thread stack allocation failed");
        free(t);
        return;
    }
    printf("intruction pointer set\n");

    /**
     * Stack grows down and malloc goes up
     * Truncating stack size with stack align
     */
    t->top = (void *) ((long)((char*)t->base + stack_size - 1) & ~STACKALIGN);
    printf("base and top set\n");
    t->arg = arg;
    t->state = READY;
    queue_add(monitor.thread_ready, t);
    printf("added to ready queue\n");
    thread_schedule();
}
void ctx_entry(void) {
    /* Invoke the a new thread and call f(), exit()
	 */
    (*monitor.current_t->ip)(monitor.current_t->arg);
    monitor.current_t->state = TERMINATED;
    queue_add(monitor.thread_exited,monitor.current_t);
    struct thread * next = findRunnable();
    if(next == NULL) {
        sys_exit(1);
    }
    thread_schedule();
    thread_exit();
}
static void thread_schedule(){
    printf("entering schedule\n");
    struct thread *old = monitor.current_t;
    struct thread *new = findRunnable();
    printf("found a runnable\n");
    if(!new) {
        sys_exit(1);
    }
    monitor.current_t = new;

    if(new->state == STOPPED) {
        if(old != new)
            ctx_switch((address_t) old->top,(address_t)&new->top);
    } else if(new->state == READY) {
        if(old != new)
            ctx_start((address_t) old->top,(address_t)&new->top);
    } else {
        //Nothing can run because the states are 'BLOCKED'
        return;
    }

    new->state = RUNNING;


}
void thread_yield() {
    //if ready queue is empty
    if(queue_empty(monitor.thread_ready)) return;
    monitor.current_t->state = STOPPED;
    queue_add(monitor.thread_ready, monitor.current_t);
    thread_schedule();
}
void thread_exit() {
    struct thread * cur;
    while((cur = queue_get(monitor.thread_exited)) != NULL) {
        if(cur->base)
            free(cur->base);
        free(cur);
    }

}
struct thread*
findRunnable(){
    struct thread *t1;
    t1 = queue_get(monitor.thread_ready);
    if(t1 == NULL) {
        // ready queue is empty
        return NULL;
    }
    printf("got a runnable!");
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
        monitor.current_t->state = BLOCKED;
        queue_add(sema->queue_wait, monitor.current_t);

    }
    thread_yield();
}
void sema_incr(struct sema* sema) {
    struct thread *thread1;
    if(++(sema->count) <= 0) {
        thread1 = (struct thread *)queue_get(sema->queue_wait);
        thread1->state = READY;
        queue_add(monitor.thread_ready,thread1);
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
int main(int argc, char **argv){
    thread_init();
    printf("thread inited\n");
    thread_create(test_code, "thread 1", 16 * 1024);
   // thread_create(test_code, "thread 2", 16 * 1024);
   // test_code("main thread");

    return 0;
}

