/*
*
*  Created by Saurabh Singh on 10/28/16.
*  Authors: Saurabh Singh,
*			Alok Singh
*           
*/

#include <stdio.h>
#include <inttypes.h>
#include "my_pthread_t.h"
#include <ucontext.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "my_mutex_and_que.h"

struct Queue queue[MAX_QUEUE_COUNT];
struct Queue waitQueue;
static int currentQueue;
struct Queue tempQueue;
static my_pthread_t *currentThread = 0;
static my_pthread_t *nextThread = 0;
static int id = 1;
struct itimerval tout_val;
int isSchedulerInitialized=0; //Call init only once-To initialize the schedular before anything else
long getCurrentTimestampNano();
long getCurrentTimestampSec();
void setAlarm(int seconds,suseconds_t microseconds);
static int waitingPriority=MAX_QUEUE_COUNT-1;
static int criticalSecTid=-1;
static int isYield= 0;
long timeSinceLastMaintainance=0;
long thisQueueQuanta;
int	mutexWasJustUnlocked=0;




/***
 * thread_start - The function that gets executed as soon as a thread is created and scheduled.
 * This function is assigned to every thread and when called with proper rguments, 
 * executes the corresponding thread function. At the end of the function we call exit as the 
 * thread has finished its work.
 * @param 	void (*t_func)(void) name of the function associated with the thread
 * @return 	null
 */
void thread_start(void (*t_func)(void)) {
	t_func();
    my_pthread_exit(NULL);
}

/** 
 * my_pthread_yelid- This function is called when a thread wants to willingly give up its current time slice (or when its waiting on a mutex)
 * Since the current thread is giving up the time slice and next thread needs to be scheduled,
 * a call to the schedular is made by raising an alarm
 * @param  null
 * @return  null
 */
void my_pthread_yield() {
	isYield = 1;
	raise(SIGALRM);
}

/** 
 *  scheduler- This is the schedular function. Every time the time sclice expires (or a thread yields or exits) and the next
 *  thread needs to be scheduled, this function is called. It switches threads between diffrent priority ques.
 *  and swaps them in and out of context.
 * 	@param int signum- registered signal handler for SIGALRM
 *  @return null
 */
void scheduler(int signum) {
	clock_gettime (CLOCK_REALTIME, (&(&queue[currentQueue])->head->thread->finish)); 
	cycle_counter++;
	//printf("Cycle cunter=%d\n",cycle_counter);
	sigset_t s;
	sigaddset(&s, SIGALRM);
	sigprocmask(SIG_BLOCK, &s, NULL);
	my_pthread_t *temp;	
	currentThread = (&queue[currentQueue])->head->thread;
	if(currentThread->firstCycle) {
		//~ printf("First cycle\n");
		currentThread->firstCycle=0;
	}
	else {
		double msecs, secs;
		secs=(double)(currentThread->finish.tv_sec-currentThread->start.tv_sec);
		if(secs==0) {	
			msecs=((double)(currentThread->finish.tv_nsec-currentThread->start.tv_nsec))/1000000;
		 }
		 else if(secs>=1) {
			secs=secs-1;
			msecs=((double)(999999999-(currentThread->start.tv_nsec))+(currentThread->finish.tv_nsec))/1000000;
		 }
		 currentThread->timeSpentSec += secs;
		 currentThread->timeSpentMsec += msecs;	
	}
	//~ printf("Time spent in secs %lf\n",currentThread->timeSpentSec);
	//~ printf("Time spent in Millisecs %lf\n",currentThread->timeSpentMsec);
	thisQueueQuanta=BASE_QUEUE_QUANTA * (currentQueue + 1);
	tout_val.it_value.tv_sec = 0; /* set timer for "INTERVAL (10) seconds */
	tout_val.it_value.tv_usec = thisQueueQuanta;
	tout_val.it_interval.tv_sec=0;
	tout_val.it_interval.tv_usec=0;
	deleteFromQueue(&queue[currentQueue],&temp);
	timeSinceLastMaintainance=timeSinceLastMaintainance+thisQueueQuanta;
	if(mutexWasJustUnlocked==1) {
		mutexWasJustUnlocked=0;
		shiftFromWaitingToReady();
	}
	if (currentThread->isFinished == 1) {
		int nextQueue= findMaxPriorityQueue();
		if(nextQueue != -1 ) {
			nextThread = queue[nextQueue].head->thread;
			// Update currentQueue value	
			currentQueue = nextQueue;
			currentThread->isCleaned = 1;		
			setitimer(ITIMER_REAL, &tout_val,0);
			setcontext(&(nextThread->context));
		} else {
			printf("All the queues are empty!!!!\n");
		}	
	} else {
			if ((isYield==1) && (currentThread->isWaiting)==1) {	
				(currentThread->yieldCount)++;
				addToQueue(temp, &waitQueue);
				//~ printf("Removing from ready queue and adding to the wait queue\n");
			}
			else if ((isYield==1) && (currentThread->timeSpentMsec)<=0.002) {	
				(currentThread->yieldCount)++;
				addToQueue(temp, &queue[currentQueue]);
				//~ printf("Keeping thread %d it at the same priority\n",currentThread->id);
			}
			else {
				int addCurrentToQueue = currentQueue==2 ? 2 : currentQueue+1;
				if(addCurrentToQueue!=currentQueue) {
					currentThread->timeSpentMsec=0;
					currentThread->yieldCount=0;
				}
				addToQueue(temp, &queue[addCurrentToQueue ]);
				//~ printf("changing the priority to queue %d of thread %d\n", addCurrentToQueue, currentThread->id);
			}
			isYield=0;		
	}
	if (timeSinceLastMaintainance >= 400000) {
		timeSinceLastMaintainance=0;
		schedule_maintainance();	
	}
	int nextQueue= findMaxPriorityQueue();	
	//printf("currentQueue: %d Current thread: %d\n",currentQueue, currentThread->id);
	currentQueue = nextQueue;
	nextThread = queue[nextQueue].head->thread;
	//printf("nextQueue: %d Next Thread: %d\n",nextQueue, nextThread->id);
	//printf("\n====================\n");
	setitimer(ITIMER_REAL, &tout_val,0);
	swapcontext(&(currentThread->context),&(nextThread->context));
	clock_gettime (CLOCK_REALTIME, (&(&queue[currentQueue])->head->thread->start)); 
	sigprocmask(SIG_UNBLOCK, &s, NULL);
}

 /**
  * my_pthread_create- This function creates threads and sets the initial values.
  * This function also calls the schedular initilization function (only once).
  * The newly created threads are added to the highest priority queue.
  * @param my_pthread_t * thread - pointer to the thread being created, 
  * void * attr- Ignored, void (*function)(void)- Function for the thread, void * arg- Ignored
  * @return int- 0 on sucess, 2 on MALLOC_ERROR
  */
int my_pthread_create(my_pthread_t * thread, void * attr, void (*function)(void), void * arg){
    if(isSchedulerInitialized == 0)
	{
		init(0,BASE_QUEUE_QUANTA);
	}
	thread->firstCycle = 1;
    thread->isFinished = 0;
    thread->isCleaned = 0;
    thread->timeSpentSec= 0;
    thread->timeSpentMsec= 0;
    thread->yieldCount=0;
    thread->start.tv_sec=0;
    thread->finish.tv_sec=0;
    thread->start.tv_nsec=0;
    thread->finish.tv_nsec=0;
    thread->isWaiting=0;
    getcontext(&(thread->context)); 
    thread->context.uc_link = 0;    //pointer to the context that will be resumed when this context returns
    thread->stack = malloc( THREAD_STACK );
    thread->context.uc_stack.ss_sp = thread->stack;
    thread->context.uc_stack.ss_size = THREAD_STACK;
    thread->context.uc_stack.ss_flags = 0;
    //Check if we can allocate memory for stack
    if ( thread->stack == 0 ) //This means malloc could not allocate memory to the stack
    {
        printf( "Error: Could not allocate stack.\n" );
        return MALLOC_ERROR;
    }
    thread->id = id++;
    addToQueue(thread, &queue[0]); //Ading the thread to the schedular queue
    makecontext( &thread->context, (void (*)(void)) &thread_start, 1, function );
    return 0;
}

/**
 * my_pthread_join- If a thread is waiting for another thread then it calls this funtion. 
 * The function checks if the thread on which the calling thread is waiting on is finished and cleaned or not.
 * If yes then the function returns and the calling function can stop waiting.
 * If not, then the calling function yields.
 * @param my_pthread_t * thread - the thread on which to wait on, void **value_ptr.
 * @return 0 if the thread on which waiting is finished
 */
int my_pthread_join(my_pthread_t * thread, void **value_ptr) {
    while (1) {
        if(thread->isCleaned == 1 && thread->isFinished == 1) {
            return 0;
        }
        else {
			my_pthread_yield();
		}
    }
}

/**
 * initMainThread- Makes sure the main thread is the first thread to be pushed to the scheduler queue
 * @param null
 * @return int 0 on sucess
 */ 
int initMainThread()
{
	my_pthread_t *mainThread = (my_pthread_t *) malloc(sizeof(my_pthread_t));
	mainThread->isFinished = 0;
    mainThread->isCleaned = 0;
    getcontext(&(mainThread->context)); //fetch the context
	mainThread->stack = mainThread->context.uc_stack.ss_sp;
	mainThread->yieldCount=0;
    mainThread->id = 0;
    mainThread->firstCycle=1;
    mainThread->timeSpentSec=0;
    mainThread->timeSpentMsec=0;
    mainThread->start.tv_sec=0;
    mainThread->finish.tv_sec=0;
    mainThread->start.tv_nsec=0;
    mainThread->finish.tv_nsec=0;
    mainThread->isWaiting=0;
    addToQueue(mainThread, &queue[0]); //Ading the thread to the schedular queue
    return 0;
}

/**
 * init - Initializes the scheduler and declares it as a signal handler.
 * Also initializes the main thread 
 * @param long int sec - the seconds component of time to set the first alarm, long int usec- the microsecond component
 * @return null
 */
void init(long int sec, long int usec)
{
	signal(SIGALRM, scheduler);
	isSchedulerInitialized=1;
	initMainThread();
	currentQueue=0;
	setAlarm(sec, usec);
}

/**
 * setAlarm- This function sets an alarm for the duration passed as a argument
 * @param int seconds the seconds component of time to set the alarm ,suseconds_t microseconds the microsecond component
 * @return null
 */
void setAlarm(int seconds,suseconds_t microseconds) {
    tout_val.it_value.tv_sec = seconds; /* set timer for "INTERVAL (10) seconds */
	tout_val.it_value.tv_usec = microseconds;
	tout_val.it_interval.tv_sec=0;
	tout_val.it_interval.tv_usec=0;
    setitimer(ITIMER_REAL, &tout_val,0);
}

/**
 * my_pthread_exit- When a thread has finished its execution, this function is called.
 * It sets the isFinished flag for the thread and calls yield function.
 * @param - void *value_ptr
 * @return null
 */
void my_pthread_exit(void *value_ptr) {
	currentThread = queue[currentQueue].head->thread;
	currentThread->isFinished = 1;
	my_pthread_yield();
}

/**
 * findMaxPriorityQueue-The function finds the queue with maximum priority that is not empty.
 * Bacically he queue from which the next thread to be scheduled is picked
 * @param null
 * @return the number of highest priorty queue that is not empty, -1 if all are empty
 */ 
int findMaxPriorityQueue() {
	int i;
	for (i=0; i<MAX_QUEUE_COUNT; i++) {
		if (!isQueEmpty(&queue[i])) {
			return i;
		}
	}
	return -1;
}

/**
 * schedule_maintainance- The function executes a periodic mantainance cycle.
 * It checks if a high priorty thread is waiting for mutex locked by a low priorty thread.
 * If yes, then it implements priorty inheritence. The thread with the mutex inherits the priorty of the 
 * higher priorty thread waiting for the mutex. This function also checks the lowest priorty queue priodically
 * and boosts the priorty of the threads stuck in the least priorty queue for long time to prevent starvation.
 * @param null
 * @return null  
 */ 
void schedule_maintainance() {
	//printf("**************Maintainance Cycle***************\n");
	if (waitingPriority < MAX_QUEUE_COUNT-1 && criticalSecTid != -1) {
			//~ printf("Invoking inheretince\n");
			inheritPriority();
	}
	struct Node *temp = queue[MAX_QUEUE_COUNT-1].head;
	temp = queue[MAX_QUEUE_COUNT-1].head;
	my_pthread_t *tempReturned;
	int tempQueFlag=0;
	//~ printf("Preventing starvation\n");
	while (temp != 0) {
			temp = temp->next;
			deleteFromQueue(&queue[MAX_QUEUE_COUNT-1],&tempReturned);
			//~ printf("Time spent in the last queue is %lf\n", tempReturned->timeSpentMsec);
			if(tempReturned->timeSpentMsec>=500){
				addToQueue(tempReturned, &queue[0]);
				//~ printf("Adding thread %d to the top queue\n", tempReturned->id );
				}
			else{
				//~ printf("Adding thread %d back to the last queue\n", tempReturned->id);
				addToQueue(tempReturned, &tempQueue);
				}
	}
	temp = tempQueue.head;
	while (temp != 0) {
		temp = temp->next;
		deleteFromQueue(&tempQueue,&tempReturned);
		addToQueue(tempReturned, &queue[MAX_QUEUE_COUNT-1]);
	}
}
/**
 * setWaitingPriority-The function check if the current thread requesting a mutex that is already locked has the higher priorty
 * than the thread currently holding the mutex.
 * If yes, the waitingPriorty is set equal to the priorty of the thread requesting the mutex.
 * waitingPriorty is used in maintainance cycle to implement priorty inheritence.
 * @param null
 * @return null
 */ 
void setWaitingPriority() {
	waitingPriority = currentQueue < waitingPriority ? currentQueue : waitingPriority;
	queue[currentQueue].head->thread->isWaiting=1;
}

/**
 * setCriticalSecTid- Sets the criticalSecTid to the thread id of the thread which just locked the mutex.
 * This is used to impliment the priority inheritence.
 * @param - null
 * @return null
 */ 
void setCriticalSecTid() {
	criticalSecTid = queue[currentQueue].head->thread->id;
	mutexWasJustUnlocked=0;
}

/**
 * inheritPriority -If a higher priorty thread is waiting on a mutex locked by a lower priorty thread, this function 
 * sets the priorty of the lower priorty thread equal to the higher priorty thread, so that the mutex can be freed asap.
 * @param null
 * @return null
 */ 
void inheritPriority() {
	int i;
	struct Node *temp = queue[0].head;
	for(i=0; i<MAX_QUEUE_COUNT; i++ ) {
		temp = queue[i].head;
		while(temp != 0) {
			if(temp->thread->id == criticalSecTid) {
				int firstTid = queue[i].head->thread->id;
				while(1) {
					my_pthread_t *temp3;
					if (queue[i].head->thread->id != criticalSecTid) {
						deleteFromQueue(&queue[i],&temp3);
						addToQueue(temp3, &queue[i]);
					} else {
						int isReturn = queue[i].head->thread->id == firstTid ? 1 : 0;
						deleteFromQueue(&queue[i],&temp3);
						addToQueue(temp3, &queue[0]);
						if (isReturn == 1) {
							return;
						}
					}
					if (queue[i].head->thread->id == firstTid) {
						return;
					}
				}
			}			
			temp = temp->next;
		}	
	}
}

/**
 * notifyMutexUnlocked- sets a flag that represents if a mutex was unlocked in the last execution quanta
 * @param null
 * @return null
 */ 
void notifyMutexUnlocked() {
	mutexWasJustUnlocked=1;
}

/**
 * shiftFromWaitingToReady- Shifts threads from the waiting queue to the ready queue if the mutex they were waiting on was unlocked
 * @param null
 * @return null
 */ 
void shiftFromWaitingToReady() {
	struct Node *temp = waitQueue.head;
	temp = waitQueue.head;
	my_pthread_t *tempReturned;
	while(temp != 0) {
		deleteFromQueue(&waitQueue,&tempReturned);
		tempReturned->isWaiting=0;
		addToQueue(tempReturned, &queue[0]);
		temp = temp->next;
	}
}
