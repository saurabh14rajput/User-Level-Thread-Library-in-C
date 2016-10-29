/*
*
*  Created by Saurabh Singh on 10/28/16.
*  Authors: Saurabh Singh,
*			Alok Singh
*           
*/

#ifndef MY_PTHREAD_T_H_
#define MY_PTHREAD_T_H_

#define MAX_QUEUE_COUNT 3
#define BASE_QUEUE_QUANTA 50000//microseconds
#define THREAD_STACK (1024*1024) //size of the stack
#define MALLOC_ERROR 2

#include <ucontext.h>

//Structure for thread
typedef struct {
    int id;
    void* stack;
    ucontext_t context;
    int isFinished;
    int isCleaned;
    int firstCycle;
    double timeSpentSec;
    double timeSpentMsec;
    struct timespec start, finish;
    int  yieldCount; 
    int isWaiting;
} my_pthread_t;

int cycle_counter;

/**
  * my_pthread_create- This function creates threads and sets the initial values.
  * This function also calls the schedular initilization function (only once).
  * The newly created threads are added to the highest priority queue.
  * @param my_pthread_t * thread - pointer to the thread being created, 
  * void * attr- Ignored, void (*function)(void)- Function for the thread, void * arg- Ignored
  * @return int- 0 on sucess, 2 on MALLOC_ERROR
  */
int my_pthread_create(my_pthread_t * thread, void * attr, void (*function)(void), void * arg);

/** 
 * my_pthread_yelid- This function is called when a thread wants to willingly give up its current time slice (or when its waiting on a mutex)
 * Since the current thread is giving up the time slice and next thread needs to be scheduled,
 * a call to the schedular is made by raising an alarm
 * @param  null
 * @return  null
 */
void my_pthread_yield();

/**
 * my_pthread_exit- When a thread has finished its execution, this function is called.
 * It sets the isFinished flag for the thread and calls yield function.
 * @param - void *value_ptr
 * @return null
 */
void my_pthread_exit(void *value_ptr);

/**
 * my_pthread_join- If a thread is waiting for another thread then it calls this funtion. 
 * The function checks if the thread on which the calling thread is waiting on is finished and cleaned or not.
 * If yes then the function returns and the calling function can stop waiting.
 * If not, then the calling function yields.
 * @param my_pthread_t * thread - the thread on which to wait on, void **value_ptr.
 * @return 0 if the thread on which waiting is finished
 */
int my_pthread_join(my_pthread_t * thread, void **value_ptr);

/***
 * thread_start - The function that gets executed as soon as a thread is created and scheduled.
 * This function is assigned to every thread and when called with proper rguments, 
 * executes the corresponding thread function. At the end of the function we call exit as the 
 * thread has finished its work.
 * @param 	void (*t_func)(void) name of the function associated with the thread
 * @return 	null
 */
 void thread_start(void (*t_func)(void));

/**
 * init - Initializes the scheduler and declares it as a signal handler.
 * Also initializes the main thread 
 * @param long int sec - the seconds component of time to set the first alarm, long int usec- the microsecond component
 * @return null
 */
void init(long int sec, long int usec);

/**
 * initMainThread- Makes sure the main thread is the first thread to be pushed to the scheduler queue
 * @param null
 * @return int 0 on sucess
 */ 
int initMainThread();

/** 
 *  scheduler- This is the schedular function. Every time the time sclice expires (or a thread yields or exits) and the next
 *  thread needs to be scheduled, this function is called. It switches threads between diffrent priority ques.
 *  and swaps them in and out of context.
 * 	@param int signum- registered signal handler for SIGALRM
 *  @return null
 */
void scheduler(int signum);

/**
 * schedule_maintainance- The function executes a periodic mantainance cycle.
 * It checks if a high priorty thread is waiting for mutex locked by a low priorty thread.
 * If yes, then it implements priorty inheritence. The thread with the mutex inherits the priorty of the 
 * higher priorty thread waiting for the mutex. This function also checks the lowest priorty queue priodically
 * and boosts the priorty of the threads stuck in the least priorty queue for long time to prevent starvation.
 * @param null
 * @return null  
 */ 
void schedule_maintainance();

/**
 * setWaitingPriority-The function check if the current thread requesting a mutex that is already locked has the higher priorty
 * than the thread currently holding the mutex.
 * If yes, the waitingPriorty is set equal to the priorty of the thread requesting the mutex.
 * waitingPriorty is used in maintainance cycle to implement priorty inheritence.
 * @param null
 * @return null
 */
void setWaitingPriority();

/**
 * setCriticalSecTid- Sets the criticalSecTid to the thread id of the thread which just locked the mutex.
 * This is used to impliment the priority inheritence.
 * @param - null
 * @return null
 */ 
void setCriticalSecTid();

/**
 * inheritPriority -If a higher priorty thread is waiting on a mutex locked by a lower priorty thread, this function 
 * sets the priorty of the lower priorty thread equal to the higher priorty thread, so that the mutex can be freed asap.
 * @param null
 * @return null
 */
void inheritPriority();

/**
 * notifyMutexUnlocked- sets a flag that represents if a mutex was unlocked in the last execution quanta
 * @param null
 * @return null
 */ 
void notifyMutexUnlocked();

/**
 * shiftFromWaitingToReady- Shifts threads from the waiting queue to the ready queue if the mutex they were waiting on was unlocked
 * @param null
 * @return null
 */  
void shiftFromWaitingToReady();
#endif
