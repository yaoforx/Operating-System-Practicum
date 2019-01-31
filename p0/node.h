//
// Created by Yao Xiao on 1/28/19.
//

#ifndef CS4411_NODE_H
#define CS4411_NODE_H

#endif //CS4411_NODE_H

#include "queue.h"
/**
 * Data structure that is implemented underneath queue to support queue operations
 * @author: Yao Xiao
 */

struct node {
    struct node* prev;
    struct node* next;
    void* item_;
};

typedef struct queue{
    struct node * head;
    struct node * tail;
    int length;
// Your code here

} queue;

typedef struct item{
    int data;
}item;

