#include "queue.h"
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
 * @return 0/-1
 */
int test_prepend(int times) {
    struct item num[times];
    struct item *iter = num;
    queue_t* new = queue_new();

    printf("Prepend 1 to %d:\n",times);
    for(int i = 1; i <= times; ++i) {

        num[i-1].data = i;

        if (-1 == queue_prepend(new, iter + i - 1)){
            printf("queue_prepend failed on i = %d\n",i);
            return -1;
        }
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

    iter = num;
    int len = times;
    while (iter != NULL && !queue_dequeue(new, (void**)&iter)) {
        assert(new->length == --len);
    }

    assert(queue_free(new) == 0);
    return 0;
}
/**
 * Test queue_append can append elem to the queue
 * @param times
 * @return 0/-1
 */
int test_append(int times){
    struct item num[times];
    struct item *iter = num;
    queue_t* new = queue_new();

    printf("Append 1 to %d:\n",times);
    for(int i = 1; i <= times; ++i) {

        num[i-1].data = i;

        if (-1 == queue_append(new, iter + i - 1)){
            printf("queu_append failed on i = %d\n",i);
            return -1;
        }
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

    iter = num;
    int len = times;
    while (iter != NULL && !queue_dequeue(new, (void**)&iter)) {
        assert(new->length == --len);
    }

    assert(queue_free(new) == 0);
    return 0;
}
/**
 * Test queue_dequeue can dequeue the first elem in the queue and return it
 * @param times
 * @return 0/-1
 */
int test_dequeue(int times) {

    item num[times];
    item *iter = num;
    queue_t* new = queue_new();

    printf("Append 1 to %d:\n",times);
    for(int i = 1; i <= times; ++i) {

        num[i-1].data = i;

        if (-1 == queue_append(new, iter + i - 1)){
            printf("queue_append failed on i = %d\n",i);
            return -1;
        }
        assert(new->length == i);
    }

    int i = 1;
    int len = times;
    while (iter != NULL && !queue_dequeue(new, (void**)&iter)) {
        // printf("Dequeue the first elem of the queue: %d\n", iter->data);
        assert(new->length == --len);
        assert(iter->data == i++);
    }

    assert(queue_free (new) == 0);

    return 0;

}
/**
 * test queue_iterate can iterate through queue
 * @param times
 * @return 0/-1
 */
int test_iterate(int times) {
    int i = 1;
    item num[times];
    item *iter = num;
    queue_t* new = queue_new();

    printf("Append 1 to %d:\n",times);
    for(int i = 1; i <= times; ++i) {

        num[i-1].data = i;

        if (-1 == queue_append(new, iter + i - 1)){
            printf("queue_append failed on i = %d\n",i);
            return -1;
        }
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
 * @return 0/-1
 */
int test_delete(int times) {

    item num[times];
    item *iter = num;
    queue_t* new = queue_new();

    printf("Append 1 to %d:\n",times);
    for(int i = 1; i <= times; ++i) {

        num[i-1].data = i;

        if (-1 == queue_append(new, iter + i - 1)){
            printf("queue_append failed on i = %d\n",i);
            return -1;
        }
        assert(new->length == i);
    }
    int err = queue_delete(new, num + 10);
    if(err == -1) {
        perror("Delete failed\n");
        return -1;
    }
    queue_iterate(new, print_queue, NULL);
    err = queue_delete(new, num + times - 1);
    if(err == -1) {
        perror("Delete failed\n");
        return -1;
    }

    queue_iterate(new, print_queue, NULL);
    err = queue_delete(new, num);
    if(err == -1) {
        perror("Delete failed\n");
        return -1;
    }

    queue_iterate(new, print_queue, NULL);
    free(new);
    return 0;


}
/**
 * This test_delete tests functionality that queue_delete can delete which ever the specified
 * first item appeared in the queue.
 * @param times
 * @return 0/-1
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


            if (-1 == queue_append(new, iter + i - 1)){
                printf("queue_append failed on i = %d\n", i);
                return -1;
            }


    }

    int err = queue_delete(new, (void**)num);//delete first 1
    if(err == -1) {
        perror("Delete failed\n");
        return -1;
    }

    err = queue_delete(new, (void**)(num + 1));//delete first 2
    if(err == -1) {
        perror("Delete failed\n");
        return -1;
    }


    err = queue_delete(new, (void**)(num + 2));//delete first 3
    if(err == -1) {
        perror("Delete failed\n");
        return -1;
    }


    err = queue_delete(new, (void**)(num + 9));//delete first 10
    if(err == -1) {
        perror("Delete failed\n");
        return -1;
    }

    queue_iterate(new, print_queue, NULL);
    free(new);
    return 0;
}

/**
 * Test both queue_prepend and queue_append
 * @param times
 * @return 0/-1
 */


int test_prepend_append(int times){
    struct item num[times];
    struct item *iter = num;
    queue_t* new = queue_new();

    printf("Append 1 to %d:\n",times);
    for(int i = 1; i <= times; ++i) {

        num[i-1].data = i;
        
        if(i%2==0){
            if (-1 == queue_append(new, iter + i - 1)){
                printf("queu_append failed on i = %d\n",i);
                return -1;
            }
        }
        else{
            if (-1 == queue_prepend(new, iter + i - 1)){
                printf("queu_prepend failed on i = %d\n",i);
                return -1;
            }
        }
            
    }

    printf("Test prepend and append\n");
    struct node * it = new->head;

    int startIndex = ((times % 2 == 0) ? (times-1):times);
    for(int i = startIndex; i >= 1; i-=2) {
        assert(((struct item *)it->item_)->data == i);
        it = it->next;
    }

    it = new->tail;
    int endIndex = ((times % 2 == 0) ? times:(times-1));
    for(int i = endIndex; i>0; i-=2) {
        assert(((struct item *)it->item_)->data == i);
        it = it->prev;
    }


    assert(new->length == times);
    printf("Prepend_Append success\n");


    iter = num;
    int len = times;
    while (iter != NULL && !queue_dequeue(new, (void**)&iter)) {
        assert(new->length == --len);
    }

    assert(queue_free(new) == 0);
    return 0;

}

/**
 * Test queue_free can free an empty queue
 * @param times
 * @return 0/-1
 */


int test_free(int times){
    item num[times];
    item *iter = num;
    queue_t* new = queue_new();

    printf("Append 1 to %d:\n",times);
    for(int i = 1; i <= times; ++i) {

        num[i-1].data = i;

        if (-1 == queue_append(new, iter + i - 1)){
            printf("queue_append failed on i = %d\n",i);
            return -1;
        }
        assert(new->length == i);
    }

    iter = num;
    int len = times;
    while (iter != NULL && !queue_dequeue(new, (void**)&iter)) {
        assert(new->length == --len);
    }

    return queue_free(new);
}
/**
 * This test tests freeing non-empty queue raises error
 * @param times
 * @return 0/-1
 */
int test_delete_nonempty(int times) {
    item num[times];
    item *iter = num;
    queue_t* new = queue_new();

    printf("Append 1 to %d:\n",times);
    for(int i = 1; i <= times; ++i) {

        num[i-1].data = i;

        if (-1 == queue_append(new, iter + i - 1)) {
            printf("queue_append failed on i = %d\n", i);
            return -1;
        }
        assert(new->length == i);
    }
    int err = queue_free(new);
    free(new);
    assert(err == -1);

    return 0;


}
/**
This test case tests dequeue over times causing failures
**/
int  test_overDequeue(int times) {
    item num[1];
    num[0].data = 1;
    queue_t * new = queue_new();
    queue_append(new, num);
    int err1 = queue_dequeue(new, (void**)&num);

    assert(err1 == 0);
    int err2 = queue_dequeue(new, (void**)&num);
    free(new);
    assert(err2 == -1);

    return 0;


}

int test_one_iterate(int times){
    item num[1];
    num[0].data = 1;
    queue_t * new = queue_new();
    queue_append(new, num);
    int err = queue_iterate(new, print_queue, NULL);

    free(new);
    return err;
}


int main(void) {
    const int test_time = 100;
    int err = 0;
    printf("Starting prepend test\n");
    if(test_prepend(test_time) == -1) return -1;
    printf("Prepend test done\n\n");

    printf("Starting append test\n");
    if(test_append(test_time) == -1) return -1;
    printf("Append test done\n\n");

    printf("Starting dequeue test with insufficient element\n");
    if(test_overDequeue(test_time) == -1) return -1;
    printf("overDequeue test done\n\n");

    printf("Starting Delete non-empty queue test\n");
    if(test_delete_nonempty(test_time) == -1) return -1;
    printf("Delete non-empty queue test done\n\n");

    printf("Starting dequeue test\n");
    if(test_dequeue(test_time) == -1) return -1;
    printf("Dequeue test done\n\n");

    printf("Starting dequeue test with insufficient element\n");
    if(test_overDequeue(test_time) == -1) return -1;
    printf("overDequeue test done\n\n");

    printf("Starting 1 element iterate test\n");
    if(test_one_iterate(test_time) == -1) return -1;
    printf("Iterate 1 element test done\n\n");

    printf("Starting iterate test\n");
    if(test_iterate(test_time) == -1) return -1;
    printf("Iterate test done\n\n");

    printf("Starting delete test\n");
    if(test_delete(test_time) == -1) return -1;
    printf("Delete test done\n\n");

    printf("Starting another delete test2\n");
    if(test_delete2(test_time) == -1) return -1;
    printf("Another Delete test done\n\n");

    printf("Starting prepend and append test\n");
    if(test_prepend_append(test_time) == -1) return -1;
    printf("Prepend and append test done\n\n");

    printf("Starting free test\n");
    if(test_free(test_time) == -1) return -1;
    printf("Free test done\n\n");

    printf("All tests pass\n\n");

    return 0;
}
