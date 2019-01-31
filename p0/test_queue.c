#include "queue.h"
#include "node.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
/* 
    This is your test file
    Write good tests!

*/
/**
 * Helper function to print the queue
 * @param i
 * @param arg
 */
void print_queue(void* i, void* arg) {
    if(((struct node*)i)->next)
        printf("%d->", ((struct item *)(((struct node*)i)->item_))->data);
    else
        printf("%d\n", ((struct item *)(((struct node*)i)->item_))->data);

}
/**
 * Test queue_prepend can prepend elem to the queue
 * @param times
 * @return 0/1
 */
int test_prepend(int times) {
    struct item num[times];
    struct item *iter = num;
    queue_t* new = queue_new();

    printf("Prepend 1 to %d:\n",times);
    for(int i = 1; i <= times; ++i) {

        num[i-1].data = i;

        if (-1 == queue_prepend(new, iter + i - 1))
            printf("queue_prepend failed on i = %d\n",i);
    }
    printf("Test prepend\n");
    struct node * it = new->tail;
    for(int i = 1; i <= times; ++i) {
        assert(((struct item *)it->item_)->data == i);
        it = it->prev;
    }
    for(int i = times; i <= 1; ++i) {
        assert(((struct item *)it->item_)->data == i);
        it = it->next;
    }


    assert(new->length == times);
    printf("prepend success\n");
    queue_free(new);



    return 0;
}
/**
 * Test queue_append can append elem to the queue
 * @param times
 * @return 0/1
 */
void test_append(int times){
    struct item num[times];
    struct item *iter = num;
    queue_t* new = queue_new();

    printf("Append 1 to %d:\n",times);
    for(int i = 1; i <= times; ++i) {

        num[i-1].data = i;

        if (-1 == queue_append(new, iter + i - 1))
            printf("queu_append failed on i = %d\n",i);
    }
    printf("Test append\n");
    struct node * it = new->head;
    for(int i = 1; i <= times; ++i) {
        assert(((struct item *)it->item_)->data == i);
        it = it->next;
    }
    for(int i = times; i <= 1; ++i) {
        assert(((struct item *)it->item_)->data == i);
        it = it->prev;
    }


    assert(new->length == times);
    printf("Append success\n");
    queue_free(new);
}
/**
 * Test queue_dequeue can dequeue the first elem in the queue and return it
 * @param times
 * @return 0/1
 */
int test_dequeue(int times) {

    item num[times];
    item *iter = num;
    queue_t* new = queue_new();

    printf("Append 1 to %d:\n",times);
    for(int i = 1; i <= times; ++i) {

        num[i-1].data = i;

        if (-1 == queue_append(new, iter + i - 1))
            printf("queue_append failed on i = %d\n",i);
        assert(new->length == i);
    }

    int i = 1;
    int len = 100;
    while (iter != NULL && !queue_dequeue(new, (void**)&iter)) {
        printf("Dequeue the first elem of the queue: %d\n", iter->data);
        assert(new->length == --len);
        assert(iter->data == i++);
    }
    queue_free (new);
    return 0;

}
/**
 * test queue_iterate can iterate through queue
 * @param times
 * @return
 */
int test_iterate(int times) {
    int i = 1;
    item num[times];
    item *iter = num;
    queue_t* new = queue_new();

    printf("Append 1 to %d:\n",times);
    for(int i = 1; i <= times; ++i) {

        num[i-1].data = i;

        if (-1 == queue_append(new, iter + i - 1))
            printf("queue_append failed on i = %d\n",i);
        assert(new->length == i);
    }
    int err = queue_iterate(new, print_queue, NULL);

    free(new);
    return err;
}
/**
 * This test_delete tests functionality that queue_delete can delete specified item
 *
 * @param times
 * @return
 */
int test_delete(int times) {

    item num[times];
    item *iter = num;
    queue_t* new = queue_new();

    printf("Append 1 to %d:\n",times);
    for(int i = 1; i <= times; ++i) {

        num[i-1].data = i;

        if (-1 == queue_append(new, iter + i - 1))
            printf("queue_append failed on i = %d\n",i);
        assert(new->length == i);
    }
    int err = queue_delete(new, num + 10);
    if(err == -1) {
        perror("Delete failed\n");
        return -1;
    }
    queue_iterate(new, print_queue, NULL);
    err = queue_delete(new, num + times - 1);
    queue_iterate(new, print_queue, NULL);
    err = queue_delete(new, num);
    queue_iterate(new, print_queue, NULL);
    free(new);
    return err;


}
/**
 * This test_delete tests functionality that queue_delete can delete which ever the specified
 * first item appeared in the queue.
 * @param times
 * @return 0/1
 */
int test_delete2(int times){
    item num[times];
    item *iter = num;
    queue_t* new = queue_new();
    for(int i = 1; i <= times/10; ++i) {
        num[i-1].data = i;

    }
    //apppend 12345678910 12345678910...10times
    for (int j = 1; j < times/10; j++)
        for(int i = 1; i <= times/10; i++) {


            if (-1 == queue_append(new, iter + i - 1))
                printf("queue_append failed on i = %d\n", i);


    }

    int err = queue_delete(new, (void**)num);//delete first 1
    if(err == -1) return -1;
    err = queue_delete(new, (void**)(num + 1));//delete first 2
    if(err == -1) return -1;
    err = queue_delete(new, (void**)(num + 2));//delete first 3
    if(err == -1) return -1;
    err = queue_delete(new, (void**)(num + 9));//delete first 10
    queue_iterate(new, print_queue, NULL);
    free(new);
    return err;
}
int main(void) {
    const int test_time = 100;
   test_prepend(test_time);
   test_append(test_time);
   test_dequeue(test_time);
   test_iterate(test_time);
    test_delete(test_time);
    test_delete2(test_time);
}
