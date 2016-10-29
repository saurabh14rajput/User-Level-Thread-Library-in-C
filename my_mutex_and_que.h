/*
*
*  Created by Saurabh Singh on 10/28/16.
*  Authors: Saurabh Singh,
*			Alok Singh
*           
*/
#ifndef MY_MUTEX_QUEUE_H_
#define MY_MUTEX_QUEUE_H_

#include "my_pthread_t.h"

/**
 * Defining data structure for mutex.
 */
typedef struct {
    int isLocked;
} my_pthread_mutex_t;

/**
 * Definig data structure for the node of the queue
 */
struct Node {
    my_pthread_t *thread;
    struct Node *next;
};
/**
 *  Definigng the data structure for the queue
 */ 
struct Queue {
    struct Node *head;
    struct Node *tail;
};

/**
 *Initializing the mutex
 */ 
int my_pthread_mutex_init(my_pthread_mutex_t *mutex);

/**
 * to lock the mutex 
 */ 
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/**
 * To unlock the mutex
 */ 
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/**
 * To destroy the mutex
 */ 
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);

/**
 * Adding thread to the queue 
 */
int addToQueue(my_pthread_t *thread,struct Queue *queue);

/**
 * Deleting a thread from the queue
 */
int deleteFromQueue(struct Queue *queue,my_pthread_t **thread);

/**
 * Called to check if a queue is empty
 */ 
int isQueEmpty(struct Queue *queue);

#endif 
