/*
*
*  Created by Saurabh Singh on 10/28/16.
*  Authors: Saurabh Singh,
*			Alok Singh
*           
*/

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <unistd.h>
#include "my_mutex_and_que.h"
#include "my_pthread_t.h"

/**
* my_pthread_mutex_init- Assigning memory to mutex and Initislizing the mutex to 0.
* @param my_pthread_mutex_t *mutex - the mutex that is to be initialized
* @return int EXIT_SUCCESS if success
*/
int my_pthread_mutex_init( my_pthread_mutex_t *mutex){
    mutex = ( my_pthread_mutex_t *) malloc(sizeof( my_pthread_mutex_t));
    mutex -> isLocked = 0;
    return(EXIT_SUCCESS);
}

/**
* my_pthread_mutex_lock- Funtion to lock the mutex
* @param my_pthread_mutex_t *mutex - the mutex to attemp lock on
* @return int EXIT_SUCCESS if success
*/
int my_pthread_mutex_lock( my_pthread_mutex_t *mutex){    
	//printf("checking if the mutex is already locked\n");
    //If the mutex is already locked,immediately yeild and put the thread in waiting state
    while(1){
        if (mutex -> isLocked){
		setWaitingPriority();
        my_pthread_yield();
        }  
    //If it is not locked, lock it for requesting thread by assigning value 1.
        else{
			//printf("Its not locked, so locking it now\n");
            mutex -> isLocked = 1;
            setCriticalSecTid();
            return(EXIT_SUCCESS);
        }
    }
}

/**
* my_pthread_mutex_unlock-Unlocking the mutex by setting the value 0.
* @param my_pthread_mutex_t *mutex
* @return int EXIT_SUCCESS if success
*/

int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex){
    if (mutex == NULL){
        my_pthread_mutex_init(mutex);
    }
    if (mutex-> isLocked){
        mutex-> isLocked = 0;
        notifyMutexUnlocked();
    }
    return(EXIT_SUCCESS);
}

/**
* my_pthread_mutex_destroy- Destroying the mutex (Unlocking it first if it is locked.)
* @param my_pthread_mutex_t *mutex
* @return int EXIT_SUCCESS if success
*/
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex){
    my_pthread_mutex_unlock(mutex);
    mutex = NULL;
    return(EXIT_SUCCESS);
}
/*-----------------------------------------------Functions for the Queue---------------------------------------------*/
/**
 * addToQueue -adding new element to the queue
 * @param my_pthread_t *thread,struct Queue *queue
 * @return int 1 on success
 */
int addToQueue(my_pthread_t *thread,struct Queue *queue){
    if(queue->head == 0){
        queue->head = malloc(sizeof(struct Node));
        queue->head->thread = thread;
        queue->tail = queue->head;
        queue->tail->next = 0;
        queue->head->next = 0;
        return 1;
    }
    else{
        queue->tail->next = malloc(sizeof(struct Node));
        queue->tail = queue->tail->next;
        queue->tail->thread = thread;
        queue->tail->next = 0;
    }
    return 1;
}

/**
 * Deleting an element from the queue
 * @param struct Queue *queue,my_pthread_t **thread
 * @return int 1 on success
 */
int deleteFromQueue(struct Queue *queue,my_pthread_t **thread){
    //check if the que is alrrady empty. Do noting if Yes
    if(queue->head == 0){
        return 0;
    }
    //Check if this is the last element in the que
    if(queue->head == queue->tail){
        *thread = queue->head->thread;
        free(queue->head);
        queue->head = 0;
        queue->tail = 0;
    }
    else{
        *thread = queue->head->thread;
        struct Node *temp;
        temp = queue->head;
        queue->head = queue->head->next;
        free(temp);
    }
    return 1;
}

/**
 * Checking if the queue is empty
 * @param struct Queue *queue
 * @return int 0 if queue is not zero, else 1
 */ 
int isQueEmpty(struct Queue *queue){
    //check if the que is alrrady empty. Do noting if Yes
     if(queue->head == 0){
        return 1;
    }
    return 0;
} 
