#include <stdio.h>
#include <ucontext.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include "types.h"
#define THRDMEM 8192

Queue* create_queue(char type){
	Queue *q = malloc(sizeof(Queue));
	q->front = NULL;
	q->rear = NULL;
	q->type = type;
	return q;
}

Thrd* create_thread(ucontext_t *context){
	Thrd *thrd = malloc(sizeof(Thrd));
	thrd->context = context;
	thrd->parent = NULL;
	thrd->next_child = NULL;
	thrd->next_ready = NULL;
	thrd->next_sem = NULL;
	thrd->joinBy = '0';
	thrd->children = create_queue('c');
	thrd->blocked = false;
	return thrd;
}

ucontext_t* create_context(){
	ucontext_t *context = malloc(sizeof(ucontext_t));
	getcontext(context);
	context->uc_link = 0;
	context->uc_flags = 0;
	context->uc_stack.ss_sp = malloc(THRDMEM);
	context->uc_stack.ss_size = THRDMEM;
	return context;
}

void enQueue(Queue *q, Thrd * t) {
	if(q->front == NULL && q->rear == NULL){
		q->front = q->rear = t;
		if(q->type == 'c'){
			t->next_child = NULL;
		}
		else if(q->type == 'r'){
			t->next_ready = NULL;
		}
		else {
			t->next_sem = NULL;
		}
		return;
	}
	if(q->type == 'c'){
		q->rear->next_child = t;
		t->next_child = NULL;
	}
	else if(q->type == 'r'){
		q->rear->next_ready = t;
		t->next_ready = NULL;
	}
	else {
		q->rear->next_sem = t;
		t->next_sem = NULL;
	}
	q->rear = t;
}

void deQueue(Queue *q) {
	if(isEmpty(q)) {
		printf("Queue is Empty\n");
		return;
	}
	if(q->front == q->rear) {
		q->front = q->rear = NULL;
	}
	else {
		Thrd *d = q->front;
		if(q->type == 'c'){
			q->front = q->front->next_child;
			d->next_child = NULL;
		}
		else if(q->type == 'r'){
			q->front = q->front->next_ready;
			d->next_ready = NULL;
		}
		else {
			q->front = q->front->next_sem;
			d->next_sem = NULL;
		}
	}
}

bool isEmpty(Queue *q){
	if(q->front == NULL && q->rear == NULL){
		return true;
	}
	else{
		return false;
	}
}

void deleteThread(Queue *q, Thrd *thrd){
	Thrd *t = q->front;
	Thrd *prev = NULL;
	if(isEmpty(q)){
		printf("Queue empty: Delete Thread");
	}
	else {
		while(t != thrd){
			prev = t;
			if(q->type == 'c'){
				t = t->next_child;
			}
			else if(q->type == 'r'){
				t = t->next_ready;
			}
			else {
				t = t->next_sem;
			}
		}
		if(t == q->front && t == q->rear){
			q->front = NULL;
			q->rear = NULL;
			if(q->type == 'c'){
				t->next_child = NULL;
			}
			else if(q->type == 'r'){
				t->next_ready = NULL;
			}
			else {
				t->next_sem = NULL;
			}
		}
		else if (q->front == t){
			if(q->type == 'c'){
				q->front = t->next_child;
				t->next_child = NULL;
			}
			else if(q->type == 'r'){
				q->front = t->next_ready;
				t->next_ready = NULL;
			}
			else {
				q->front = t->next_sem;
				t->next_sem = NULL;
			}
		}
		else if(q->rear == t){
			if(q->type == 'c'){
				prev->next_child = NULL;
			}
			else if(q->type == 'r'){
				prev->next_ready = NULL;
			}
			else {
				prev->next_sem = NULL;
			}
			q->rear = prev;
		}
		else{
			if(q->type == 'c'){
				prev->next_child = t->next_child;
				t->next_child = NULL;
			}
			else if(q->type == 'r'){
				prev->next_ready = t->next_ready;
				t->next_ready = NULL;
			}
			else {
				prev->next_sem = t->next_sem;
				t->next_sem = NULL;
			}
		}
	}
}

void cleanThread(Thrd *thrd){
	Thrd *c = thrd->children->front;
	while(c != NULL){
		c->parent = NULL;
		c = c->next_child;
	}
	if(thrd->parent != NULL){
		deleteThread(thrd->parent->children, thrd);
		thrd->parent = NULL;
	}
	free(thrd->children);
	free(thrd->context);
	free(thrd);
}
