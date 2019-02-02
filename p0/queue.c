/*
 * Generic queue implementation.
 *
 */
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>

queue_t*
queue_new() {
	queue_t* queue1 = malloc(sizeof(queue_t));
	if(queue1 == NULL){
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
	if(queue == NULL || item == NULL){
		perror("Illegal inputs");
        return -1;
	}

    struct node* newNode = malloc(sizeof(struct node));
    if(newNode == NULL){
    	perror("Prepend failed due to allocation error");
    	return -1;
    }

    newNode->item_ = item;

    if(queue->length == 0){	
    	queue->tail = newNode;  	
    	queue->tail->next = NULL;
    }
    else{
    	queue->head->prev = newNode;
    	newNode->next = queue->head;
    }
    
    queue->head = newNode;
    queue->head->prev = NULL;
    queue->length++;
    
    return 0;

}

int
queue_append(queue_t *queue, void* item) {
	if(queue == NULL || item == NULL){
		perror("Illegal inputs");
        return -1;
	}

    struct node* newNode = malloc(sizeof(struct node));
    if(newNode == NULL){
    	perror("Append failed due to allocation error");
    	return -1;
    }

    newNode->item_ = item;

    if(queue->length == 0){	
    	queue->head = newNode;  	
    	queue->head->prev = NULL;
    }
    else{
    	queue->tail->next = newNode;
    	newNode->prev = queue->tail;
    }
    
    queue->tail = newNode;
    queue->tail->next = NULL;
    queue->length++;
    
    return 0;

}

int
queue_dequeue(queue_t *queue, void** item) {
	if(queue == NULL || item == NULL || queue->length == 0) {
        *item = NULL;
        return -1;
    }

	*item = queue->head->item_;
	if(queue->length == 1)
		queue->head = queue->tail = NULL;
	else{
		queue->head = queue->head->next;
		queue->head->prev = NULL;
	}

	queue->length--;
	return 0;
}

int
queue_iterate(queue_t *queue, func_t f, void* arg) {
	if(queue == NULL || f == NULL){
        perror("Illegal inputs");
        return -1;
	}

	for(struct node* cur = queue->head; cur != NULL; cur = cur->next){
		f(cur, arg);
	}

    return 0;
}

int
queue_free (queue_t *queue) {
	if(queue == NULL || queue->length != 0){
		perror("Illegal inputs");
		return -1;
	}
    free(queue);
	return 0;

}

int
queue_length(const queue_t *queue) {
    return (queue == NULL)? -1:queue->length;
}

int
queue_delete(queue_t *queue, void* item) {
	if(queue == NULL || item == NULL || queue->length == 0){
        perror("Illegal inputs");
		return -1;
	}

	struct node* q = queue->head;
	printf("To delete %d\n", ((struct item *) item)->data);

	while(q && q->item_ != item){
		q = q->next;
	}

	if(q == NULL){
		perror("Item is not found");
		return -1;
	} 

	if(q->prev && q->next) {
        q->prev->next = q->next;
        q->next->prev = q->prev;

    } 
    else if(q->next){
        queue->head = q->next;
        queue->head->prev = NULL;
    }
    else if(q->prev) {
        queue->tail = q->prev;
        queue->tail->next = NULL;
    }

    printf("Deleted %d\n", ((struct item *) item)->data);


    return 0;

	
}
