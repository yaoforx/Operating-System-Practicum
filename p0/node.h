//
// Created by Yao Xiao on 1/28/19.
//

#ifndef NODE_H
#define NODE_H

#endif //NODE_H

/**
 * Data structure that is implemented underneath a queue to support queue operations
 * @author: Yao Xiao & Lijie Tu
 */

struct node {
    struct node* prev;
    struct node* next;
    void* item_;
};


typedef struct item{
    int data;
}item;

