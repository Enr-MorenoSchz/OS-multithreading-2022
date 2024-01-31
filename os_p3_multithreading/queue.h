#ifndef HEADER_FILE
#define HEADER_FILE


typedef struct element{
	int type; //Machine type
	int time; //Using time
}element;

typedef struct queue{
	int start; 		//Index used as a pointer inside the queue to know where to insert the next operation.
	int end;   		//Index used as a pointer inside the queue to know from where to extract the next operation.
	int num_elements;   	//Number of elements already placed into the queue.
	int total_slots;	//Number of slots the queue has to store elements in it.
	element* content;	//Array of element datatype whose slots are used to contain the requested input operations.
}queue;

queue* queue_init(int size);
int queue_destroy(queue *q);
int queue_put(queue *q, element* elem);
element * queue_get(queue *q);
int queue_empty(queue *q);
int queue_full(queue *q);

#endif
