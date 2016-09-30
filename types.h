#ifndef TYPES_H
#define TYPES_H

typedef struct Thrd Thrd;

typedef struct Queue Queue;

typedef struct Semaphore Semaphore;

struct Thrd {
	ucontext_t *context;
	Thrd *next_child;
	Thrd *next_ready;
	Thrd *next_sem;
	Thrd *parent;
	Queue *children;
	char joinBy;
	bool blocked;
};

struct Queue {
	char type;
	Thrd *front;
	Thrd *rear;
};

struct Semaphore{
	int value;
	Queue *waitQueue;
};

Queue* create_queue(char type);

Thrd* create_thread(ucontext_t *context);

ucontext_t* create_context();

void enQueue(Queue *q, Thrd * t);

void deQueue(Queue *q);

bool isEmpty(Queue *q);

void deleteThread(Queue *q, Thrd *thrd);

void cleanThread(Thrd *thrd);

#endif
