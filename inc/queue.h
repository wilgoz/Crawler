#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>

typedef struct node node_t;
typedef struct queue {
    size_t  size;
    node_t *head;
    node_t *tail;
} queue_t;

queue_t *init_queue ();
int  	 q_empty    (const queue_t *q);
void 	 free_queue (queue_t *q);
int  	 dequeue    (queue_t *q, char **res);
int  	 enqueue    (queue_t *q, const char *data);

#endif