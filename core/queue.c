#include "../inc/queue.h"
#include "../inc/macros.h"

#include <string.h>

typedef struct node {
    char   *data;
    node_t *next;
} node_t;

queue_t *init_queue()
{
    queue_t *ptr = malloc(sizeof(*ptr));
    if (!ptr) handle_error("malloc init queue");
    ptr->head = ptr->tail = NULL;
    ptr->size = 0;
    return ptr;
}

int enqueue(queue_t *q, const char *data)
{
    node_t *new_node = malloc(sizeof(*new_node));
    if (!new_node) handle_error("malloc enqueue");

    new_node->data = strdup(data);
    new_node->next = NULL;

    if (q->size++ == 0)
        q->head = q->tail = new_node;
    else
        q->tail = q->tail->next = new_node;
    return 0;
}

int dequeue(queue_t *q, char **res)
{
    if (q->size > 0)
    {
        node_t *temp = q->head;
        *res = strdup(temp->data);

        if (q->size-- > 1)
            q->head = q->head->next;
        else
            q->head = q->tail = NULL;

        free(temp->data);
        free(temp);
        return 0;
    }
    return -1;
}

void free_queue(queue_t *q)
{
    node_t *temp;
    while (q->size-- > 0)
    {
        temp = q->head;
        q->head = temp->next;
        free(temp->data);
        free(temp);
    }
    free(q);
}

int q_empty(const queue_t *q) { return q->size == 0; }