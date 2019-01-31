/*
 * Generic queue implementation.
 *
 */
//#include "queue.h"
#include "node.h"
#include <stdlib.h>
#include <stdio.h>

queue_t*
queue_new() {

    queue_t * queue1 = malloc(sizeof(queue_t));

    if(queue1 == NULL) {
        perror("Allocation falied");
        return NULL;
    }
    queue1->length = 0;
    queue1->head = NULL;
    queue1->tail = NULL;

    return queue1;
}

int
queue_prepend(queue_t *queue, void* item) {
    //maprintf("0hey");
    if(item == NULL || queue == NULL)
        return -1;
    struct node * newNode = malloc(sizeof(struct node));
    if(newNode == NULL) {
        perror("Insertion failed due to allocation error");
        return -1;
    }
    newNode->item_ = item;

    if(queue->length == 0) {

        queue->head = newNode;

        queue->tail = newNode;


    } else {
        queue->head->prev = newNode;
        newNode->next = queue->head;
        newNode->prev = NULL;
        queue->head = newNode;
    }
    queue->length += 1;
    queue->tail->next = NULL;
    queue->head->prev = NULL;

    return 0;

}

int
queue_append(queue_t *queue, void* item) {
    if(item == NULL || queue == NULL)
        return -1;
    struct node * newNode = malloc(sizeof(struct node));
    if(newNode == NULL) {
        perror("Insertion failed due to allocation error");
        return -1;
    }

    newNode->item_ = item;

    if(queue->length == 0) {
        queue->head = newNode;
        queue->tail = newNode;

    } else {
        queue->tail->next = newNode;
        newNode->next = NULL;
        newNode->prev =queue->tail;
        queue->tail = newNode;

    }
    queue->length += 1;
    queue->tail->next = NULL;
    queue->head->prev = NULL;
    return 0;
}

int
queue_dequeue(queue_t *queue, void** item) {
    if(queue == NULL || item == NULL || queue->head == NULL) {
        *item = NULL;
        return -1;
    }
    if(queue->length == 0) {
        *item = NULL;
        printf("The queue is empty.\n");
        return 0;
    }
    *item = queue->head->item_;
    struct node* ptr = queue->head;
    if(queue->head->next == NULL) {
        queue->head = NULL;
        queue->tail = NULL;


    } else {
        queue->head = queue->head->next;
        queue->head->prev = NULL;
    }
    queue->length -= 1;



    return 0;

}

int
queue_iterate(queue_t *queue, func_t f, void* arg) {

    if(queue == NULL || f == NULL) return -1;
    for(struct node * cur = queue->head; cur != NULL; cur = cur->next) {
        f(cur, arg);
    }
    return 0;
}

int
queue_free (queue_t *queue) {
    if(queue == NULL || queue->head == NULL)
        return -1;
    struct node* next;

    free(queue);
    return 0;

}

int
queue_length(const queue_t *queue) {
    if (queue == NULL)
        return -1;
    return queue->length;

}

int
queue_delete(queue_t *queue, void* item) {
    if(queue == NULL || item == NULL )
        return -1;
    struct node* ptr = item;
    struct node* q = queue->head;
    printf("%d\n", ((struct item *) item)->data);
    while(q != NULL && ptr != q->item_) {
        q = q->next;
    }

    if(q == NULL) {
        perror("Item is not found");
        return -1;
    }
    if(q->prev) {
        q->prev->next = q->next;

    } else {
        queue->head = q->next;

    }
    if(q->next) {
        q->next->prev = q->prev;
    } else {
        queue->tail = q->prev;
    }

    free(q);
    q->prev = NULL;
    q->next = NULL;
    return 0;

}
