#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include "mythread.h"
#include "types.h"
#define THRDMEM 8192

ucontext_t Main;

Thrd *running;

Queue *ready_q;

ucontext_t* foo(){
	ucontext_t *fallBack = malloc(sizeof(ucontext_t));
	getcontext(fallBack);
	fallBack->uc_link = 0;
	fallBack->uc_flags = 0;
	fallBack->uc_stack.ss_sp = malloc(THRDMEM);
	fallBack->uc_stack.ss_size = THRDMEM;
	makecontext(fallBack, (void *)MyThreadExit, 0);
	return fallBack;
}

// ****** CALLS ONLY FOR UNIX PROCESS ******
// Create and run the "main" thread
void MyThreadInit(void(*start_funct)(void *), void *args){
	ready_q = create_queue('r');
	ucontext_t *context = create_context();
	context->uc_link = foo();
	makecontext(context, (void*)start_funct, 1, args);
	running = create_thread(context);
	swapcontext(&Main, running->context);
}

MyThread MyThreadCreate(void(*start_funct)(void *), void *args){
	ucontext_t *context = create_context();
	context->uc_link = foo();
	makecontext(context, (void*)start_funct, 1, args);
	Thrd *thrd = create_thread(context);
	thrd->parent = running;
	enQueue(running->children, thrd);
	enQueue(ready_q, thrd);
	return (void*)thrd;
}

// Yield invoking thread
void MyThreadYield(void){
	if(isEmpty(ready_q)){
		//printf("Empty Ready queue!: MyThreadYield");
	}
	else {
		Thrd *oldRunning = running;
		running = ready_q->front;
		enQueue(ready_q, oldRunning);
		deQueue(ready_q);
		swapcontext(oldRunning->context, running->context);
	}
}

// Join with a child thread
int MyThreadJoin(MyThread thread){
	Thrd *t = (Thrd *)thread;
	if(t->parent == running){
		t->joinBy = 'p';
		Thrd *blocked = running;
		running = ready_q->front;
		deQueue(ready_q);
		blocked->blocked = true;
		swapcontext(blocked->context, running->context);
		return 0;
	}
	else {
		return -1;
	}
}

// Join with all children
void MyThreadJoinAll(void){
	if(isEmpty(ready_q)){
		//printf("Empty Ready queue! : MyThreadJoinAll");
	}
	else {
		Thrd *t = running->children->front;
		while(t != NULL){
			t->joinBy = 'a';
			t = t->next_child;
		}
		Thrd *blocked = running;
		running = ready_q->front;
		deQueue(ready_q);
		blocked->blocked = true;
		swapcontext(blocked->context, running->context);
	}
}

// Terminate invoking thread
void MyThreadExit(void){
	Thrd *exiting = running;
	if((exiting->joinBy == 'p') || (exiting->joinBy == 'a' && exiting->parent->children->front->next_child == NULL)){
		//Unblock parent and put in ready queue
		exiting->parent->blocked = false;
		enQueue(ready_q, exiting->parent);
	}
	cleanThread(exiting);
	if(isEmpty(ready_q)){
		setcontext(&Main);
	}
	else{
		running = ready_q->front;
		deQueue(ready_q);
		setcontext(running->context);
	}
}
// ****** SEMAPHORE OPERATIONS ******
// Create a semaphore
MySemaphore MySemaphoreInit(int initialValue){
	if(initialValue < 0){
		return NULL;
	}
	Semaphore *s = malloc(sizeof(Semaphore));
	s->value = initialValue;
	s->waitQueue = create_queue('s');
	return (void*)s;
}

// Signal a semaphore
void MySemaphoreSignal(MySemaphore sem){
	Semaphore *s = (Semaphore *)sem;
	if(isEmpty(s->waitQueue)){
		s->value = s->value + 1;
	}
	else{
		s->value = s->value + 1;
		Thrd *nextThrd = s->waitQueue->front;
		nextThrd->blocked = false;
		enQueue(ready_q, nextThrd);
		deQueue(s->waitQueue);
	}
}

// Wait on a semaphore
void MySemaphoreWait(MySemaphore sem){
	Semaphore *s = (Semaphore *)sem;
	if(s->value > 0){
		s->value = s->value - 1;
	}
	else{
		s->value = s->value - 1;
		Thrd *blocked = running;
		running = ready_q->front;
		deQueue(ready_q);
		blocked->blocked = true;
		enQueue(s->waitQueue, blocked);
		swapcontext(blocked->context, running->context);
	}
}

// Destroy on a semaphore
int MySemaphoreDestroy(MySemaphore sem){
	Semaphore *s = (Semaphore *)sem;
	if(isEmpty(s->waitQueue)){
		free(s->waitQueue);
		free(s);
		return 0;
	}
	else{
		return -1;
	}
}
