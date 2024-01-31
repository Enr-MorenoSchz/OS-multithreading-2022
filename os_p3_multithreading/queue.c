


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "queue.h"



//To create a queue
queue* queue_init(int size){
	queue* q = (queue*) malloc(sizeof(queue));
	
	//Set default values for the queue.
	q -> start = 0;
	q -> end = 0;
	q -> num_elements = 0;
	q -> total_slots = size;
	
	//Allocate enough space for storing the maximum number of elements indicated in the input.
	q -> content = malloc((q -> total_slots) * sizeof(element));
	return q;
}


// To Enqueue an element
int queue_put(queue *q, element* x){

	//Store the input element into the queue.
	q -> content[(q -> start)] = *x;
	
	//Point to the new position to insert the next element. Use module operation to guarantee the correctness of the buffer.	
	q -> start = ((q -> start) + 1) % (q -> total_slots);
	
	//Increase the counter of elements stored in the circular buffer.
	(q -> num_elements)++;
	return 0;
}


// To Dequeue an element.
element* queue_get(queue *q){

	//Extract the element from the buffer.
	element* element = &(q -> content[(q -> end)]);
	
	//Point the new position from where we can extract the next element. Use module operation to guarantee the correctness of 
	//the buffer.
	q -> end = ((q -> end) + 1) % (q -> total_slots);
	
	//Decrease the counter of elements stored in the circular buffer.
	(q -> num_elements)--;
	return element;
}


//To check queue state
int queue_empty(queue *q){
	if ((q -> num_elements) == 0){
		return 1;
	}
	return 0;
}

int queue_full(queue *q){
	if ((q -> num_elements) == (q -> total_slots)){
		return 1;
	}	
	return 0;
}

//To destroy the queue and free the resources
int queue_destroy(queue *q){
	
	//First, free the content inside the queue. Afer that, free the queue itself.
	free(q -> content);
	free(q);
	return 0;
}
