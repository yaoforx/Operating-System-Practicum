#include "queue.h"
#include "node.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
/* 
    This is your test file
    Write good tests!

*/

void print_queue(void* node, void* arg) {
    printf("%d->", ((struct item*)node)->data);
}
// Test 1 is given to you

int test_prepend(int times) {
    struct item num[times];
    struct item *iter = num;
    queue_t* new = queue_new();

    printf("Prepend 1 to %d:\n",times);
    for(int i = 1; i <= times; ++i) {
        printf("prepending: %d\n", i);
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
void test_append(int times){
    struct item num[times];
    struct item *iter = num;
    queue_t* new = queue_new();

    printf("Append 1 to %d:\n",times);
    for(int i = 1; i <= times; ++i) {
        printf("Append: %d\n", i);
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
int main(void) {
    const int test_time = 100;
    test_prepend(test_time);
    test_append(test_time);
}
