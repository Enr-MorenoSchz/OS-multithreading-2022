
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

//Global variable to become the intermediate buffer between producers and consumers.
queue* circular_queue;

//Arrays of integers to store the type of machine and used time of that machine in each operation respectively.
int* type_machine_array;
int* time_machine_array;

//Mutex to access the queue.
pthread_mutex_t mutex;

//Condition variables related to the state of the queue.
pthread_cond_t non_full, non_empty;

//Condition variable to inform about the complete creation of a thread (NOTE: It is only used for producers as there is no actual
//need of use it for the consumer creation, since the input parameter the consumer receives is always the same and does not vary).
pthread_cond_t started;

//Global variable storing the number of operations already computed by the consumer threads.
int total_computations;

//Global variable, used as a boolean, where 0 means there are still operations to be proccessed and therefore do not inform the
//consumer threads to exit. Otherwise, this variable will receive the value 1.
int finish_consumers_exec = 0;

//Global variable indicating if the thread has been completely created (NOTE: Only used for producer threads). It will be used
//as a boolean, where 0 means the thread has not started yet its normal execution, since it is still being created; whereas 1
//means it has completed its creation and starts running normally.
int has_started = 0;

//Structure to store the range of operations a thread has to produce.
typedef struct operations_range{
	int initial_operation, final_operation;
}operations_range;



//Function for producing the elements to be stored in the queue.
void* producer_func(operations_range* producer_range){
	
	//Create a variable to store the element created (i.e. its data regarding the machine type and time of use).
	element operation_structure;
	
	//Take the mutex to guarantee the correct creation of the producer thread. That is, it receives the correct input.
	if (pthread_mutex_lock(&mutex) < 0){
		perror("ERROR: Mutex lock could not be correctly executed.\n");
		exit(-1);		
	}
	
	int initial_position = (producer_range -> initial_operation);
	int final_position = (producer_range -> final_operation);
	
	//Inform that the producer thread has been correctly created.
	has_started = 1;
	
	//Produce the signal started to let the main program continue creating the next producer thread. Then, release the mutex.
	if (pthread_cond_signal(&started)){
		perror("ERROR: Producing the signal for the started condition variable was not correctly done.\n");
		exit(-1);
	}
	if (pthread_mutex_unlock(&mutex) < 0){
		perror("ERROR: Mutex unlock could not be correctly executed.\n");
		exit(-1);
	}
	
	//Start generating the elements and store them into the queue.
	for (int i = initial_position; i < final_position; i++){
		operation_structure.type = type_machine_array[i];
		operation_structure.time = time_machine_array[i];
		
		//Take the mutex again to modify the content of the queue.
		if (pthread_mutex_lock(&mutex) < 0){
			perror("ERROR: Mutex lock could not be correctly executed.\n");
			exit(-1);		
		}
		
		//Check if the queue is full. It that is the case, wait until a consumer thread leaves a free slot.
		while(queue_full(circular_queue) == 1){
			if (pthread_cond_wait(&non_full, &mutex) < 0){
				perror("ERROR: Waiting for a condition variable could not be correctly done.\n");
				exit(-1);
			}
		}
		
		//Insert the created element into the free slot.
		queue_put(circular_queue, &operation_structure);
		
		//Inform that the queue is not empty, so that any consumer that is waiting takes that element.
		if (pthread_cond_signal(&non_empty) < 0){
			perror("ERROR: Producing the signal for the non_empty condition variable was not correctly done.\n");
			exit(-1);
		}
		
		//Release the mutex.
		if (pthread_mutex_unlock(&mutex) < 0){
			perror("ERROR: Mutex unlock could not be correctly executed.\n");
			exit(-1);
		}
	}
	
	//Finish the producer thread execution.
	pthread_exit(0);
}



//Function for extracting the elements stored in the queue and perform the partial cost computation.
void* consumer_func(){

	//Create 2 variables: the first is a int variable to store the cost, while the second is a pointer to an integer where
	//we will store the content of the first one. Also, create a variable pointing to an element data structure where to
	//store the element taken from the queue.
	int partial_cost; 
	int* partial_cost_storage = malloc(sizeof(int));
	element* operation_structure;
	
	//Create an infinite loop.
	for(;;){
	
		//Take the mutex.
		if (pthread_mutex_lock(&mutex) < 0){
			perror("ERROR: Mutex lock could not be correctly executed.\n");
			exit(-1);		
		}

		//Check if the queue is empty. If that is the case, then check if it is actually empty or if we must finish the
		//execution of the consumer thread.
		while(queue_empty(circular_queue) == 1){
			
			//Check if the execution must finish.
			if (finish_consumers_exec == 1){
				
				//Release the mutex.
				if (pthread_mutex_unlock(&mutex) < 0){
					perror("ERROR: Mutex unlock could not be correctly executed.\n");
					exit(-1);
				}
				
				//Store the content from partial_cost into partial_cost_storage. Then, return that pointer.
				*partial_cost_storage = partial_cost;
				pthread_exit((void*)partial_cost_storage);
			}
			
			//If the queue is just empty (i.e. finish_consumers_exec == 0), wait until a producer thread produces the
			//non_empty signal.
			if (pthread_cond_wait(&non_empty, &mutex) < 0){
				perror("ERROR: Waiting for a condition variable could not be correctly done.\n");
				exit(-1);
			}
		}
		
		//Take an element from the queue.
		operation_structure = queue_get(circular_queue);
		
		//Check the type of machine to be used, so we can compute the cost of using that machine for the specified time.
		//NOTE: In case the time specified in the element is negative, we use abs() to convert it into posivite.
		switch(operation_structure -> type){
			case 1:
				partial_cost += (abs(operation_structure -> time) * 3);
				break;
			case 2:
				partial_cost += (abs(operation_structure -> time) * 6);
				break;
			case 3:
				partial_cost += (abs(operation_structure -> time) * 15);
				break;
			default:
				perror("ERROR: Machine type not valid.\n");
		}
		
		//Increase the number of total performed computations.
		total_computations++;
		
		//Produce the non_full signal to inform any producer thread that is waiting that there is already a free slot in
		//the queue. After that, release the mutex.
		if (pthread_cond_signal(&non_full) < 0){
			perror("ERROR: Producing the signal for the non_full condition variable was not correctly done.\n");
			exit(-1);
		}
		if (pthread_mutex_unlock(&mutex) < 0){
			perror("ERROR: Mutex unlock could not be correctly executed.\n");
			exit(-1);
		}	
	}
}



/**
 * Entry point
 * @param argc
 * @param argv
 * @return
 */
int main (int argc, const char * argv[] ) {

	//Declare variables to store the input data regarding number of producers, consumers and maximum size for the queue.
	int num_producer, num_consumer, queue_size;
	
	//Check if the number of received parameters is correct (i.e. 5 parameters (including the name of the program)).
	if (argc != 5){
		if (argc < 5){
			perror("ERROR: There are missing arguments.\n");
			return -1;
		}
		else{
			perror("ERROR: Too many arguments.\n");
			return -1;
		}
	}
	
	//Check if the received parameters belong to the respective expected types.
	
	//First, check if the input file can be opened and if its content satisfies the requirements to be used in the program.
	//Due to the need of reading each line of the file to check the requirements for the file, and the Lab document does not
	//ban the use of any specific function to read the file, we will use functions fopen(), fscanf(), fgetc() and feof().
		
	//Create a file descriptor variable pointing to a FILE datatype.
	FILE* fd;
	
	//Open the input file.
	if ((fd = fopen(argv[1], "r")) == NULL){
		perror("ERROR: File could not be opened.\n");
		return -1;
	}
	
	//Now, check the content of the input file.
	
	//First, obtain the number of operations in the first line of the file by using fscanf().
	int num_operations, num_lines;
	char line_operation;
	
	//Store into num_operations the number read in the first line of the file.
	if (fscanf(fd, "%d", &num_operations) < 0){
		perror("ERROR: fscanf() could not be correctly executed.\n");
		return -1;
	}
	
	//Then, check if the total amount of operations in the file is equal or greater (never less) than num_operations variable.
	//For that, use the functions feof() and fgetc(), where the first let us know if we have reached the end of file; whereas
	//the second let us obtain the content from each line.
	while(!feof(fd)){
		line_operation = fgetc(fd);
		if (line_operation == '\n') num_lines++;
	}
	
	//Check if the number of operations to be computed is greater than the number of read lines in the file. If that is the
	//case, then we finish the program.
	if ((num_operations - 1) > num_lines){
		perror("ERROR: Number of operation lines are less than total number of operations to be computed.\n");
		return -1;
	}
	
	//Check the number of producer and consumers are valid.
	if ((num_producer = atoi(argv[2])) <= 0){
		perror("ERROR: Number of producers is not valid.\n");
		return -1;
	}
	if ((num_consumer = atoi(argv[3])) <= 0){
		perror("ERROR: Number of consumers is not valid.\n");
		return -1;
	}
	
	//Check the size for the queue (number of maximum elements it can store) is valid.
	if ((queue_size = atoi(argv[4])) <= 0){
		perror("ERROR: The input size for the queue is not valid.\n");
		return -1;
	}
	
	//Now, initialize the mutex and the condition variables
	if (pthread_mutex_init(&mutex, NULL) < 0){
		perror("ERROR: Mutex could not be initialized.\n");
		return -1;
	}
	if (pthread_cond_init(&non_full, NULL) < 0){
		perror("ERROR: non_full condition variable could not be initialized.\n");
		return -1;
	}
	if (pthread_cond_init(&non_empty, NULL) < 0){
		perror("ERROR: non_empty condition variable could not be initialized.\n");
		return -1;
	}
	if (pthread_cond_init(&started, NULL) < 0){
		perror("ERROR: started condition variable could not be initialized.\n");
		return -1;
	}
	
	
	//We need to return to the start of the input file and then skip the first line (the one containing the number of
	//operations) so we can start storing each column in an array. We will use the function fseek() and fscanf() for this
	//first purpose.

	//Return to the starting position of the input file.
	if (fseek(fd, 0, SEEK_SET) != 0){
		perror("ERROR: fseek() could not be correctly executed while returning to starting position in input file.\n");
		return -1;
	}

	//Skip the first line of the file (i.e. the number of operations to be executed).
	fscanf(fd, "%d", &num_operations);

	//Allocate space in the dynamic memory to store all the machine types and use time of each operation in their respective
	//data vector.
	type_machine_array = (int*) malloc(num_operations * sizeof(int));
	time_machine_array = (int*) malloc(num_operations * sizeof(int));

	//Start storing the information from the file into the vectors.
	int machine_type, machine_time;
	for (int i = 0; i < num_operations; i++){
		//We read the content of each line and guarantee that it follows the correct structure of 3 integers (1st the 
		//line ID, 2nd the Machine type, and 3rd the Time of Use).
		if (fscanf(fd, "%*d %d %d", &machine_type, &machine_time) < 0){
			perror("ERROR: Operation data could no be correctly extracted.\n");
			return -1;
		}
		
		//Store the content of the read line into their respective array.
		type_machine_array[i] = machine_type;
		time_machine_array[i] = machine_time;
	}
	
	//Close the input file as it will not be used anymore. In order to do so, we use the function fclose().
	if (fclose(fd) < 0){
		perror("ERROR: Input file could not be closed.\n");
		return -1;
	}

	//Initialize the queue.
	circular_queue = queue_init(queue_size);
	
	//Create the producer threads. In order to do so, we also need to distribute the operations among the N producers.
	//NOTE: The operations may not be equally distributed, since the division of the number of operations by N producers
	//could have a reminder != 0. Therefore, we have to provide a solution for that situation. 
	
	//Create 2 arrays. The first storing the created producer threads, and the second storing their ID line ranges.
	pthread_t producers[num_producer];
	operations_range ranges_list[num_producer];
	
	//Compute the quotient and reminder.
	int operations_per_producer = num_operations / num_producer;
	int remaining_operations = num_operations - (num_producer * operations_per_producer);
	
	//Create a variable to specify from which ID line the producer will start producing elements.
	int starting_index = 0;
	
	for (int i = 0; i < num_producer; i++){
	
		//Establish the range for each producer thread.
		if (remaining_operations == 0){
			ranges_list[i].initial_operation = starting_index;
			ranges_list[i].final_operation = starting_index + operations_per_producer;
			starting_index += operations_per_producer;
		}
		else{
			ranges_list[i].initial_operation = starting_index;
			ranges_list[i].final_operation = starting_index + operations_per_producer + 1;
			starting_index += (operations_per_producer + 1);
			remaining_operations--;
		}
		
		//Create the producer thread.
		if (pthread_create(&(producers[i]), NULL, (void*) producer_func, (void*)&(ranges_list[i])) < 0){
			perror("ERROR: Producer thread could not be created.\n");
			return -1;
		}
		
		//Take the mutex.
		if (pthread_mutex_lock(&mutex) < 0){
			perror("ERROR: Mutex lock could not be correctly executed.\n");
			return -1;
		}
		
		//Check if the producer thread has finished its creation.
		while(!has_started){
			if (pthread_cond_wait(&started, &mutex) < 0){
				perror("ERROR: Waiting for a condition variable could not be correctly done.\n");
				return -1;
			}
		}
		
		//Restore the has_started variable to 0, so that it can be used for the next producer thread creation.
		has_started = 0;
		
		//Release the mutex.
		if (pthread_mutex_unlock(&mutex) < 0){
			perror("ERROR: Mutex unlock could not be correctly executed.\n");
			return -1;
		}
	}

	//Create the M consumer threads. In comparison to the producer threads, the Lab document does not specify that we had to
	//distribute equally the operations entering in the queue among the consumer threads, so we do not need to have any
	//kind of counter to know the number of operations a consumer thread has computed. However, we need to count the number
	//of operations that have been already computed at every moment. This counter is the global variable total_computations.
	
	
	//Create an array of threads to store the consumer threads.
	pthread_t consumers[num_consumer];
	for (int i = 0; i < num_consumer; i++){
		if(pthread_create(&(consumers[i]), NULL, (void*) consumer_func, NULL)){
			perror("ERROR: Consumer thread could not be created.\n");
			return -1;
		}
	}
	
	//Wait until all threads (both producers and consumers) have finished.
	
	//First, wait until all producer threads have finished.
	for (int i = 0; i < num_producer; i++){
		if (pthread_join(producers[i], NULL) < 0){
			perror("ERROR: The finalization of a producer thread could not be correctly received.\n");
			return -1;
		}
	}
	
	//Check if the number of operations to be computed have been reached.
	while(total_computations != num_operations);
	
	//Take the mutex.
	if (pthread_mutex_lock(&mutex) < 0){
		perror("ERROR: Mutex lock could not be correctly executed.\n");
		return -1;		
	}
	
	//Let the consumer threads know that it is the time to finish their execution. Also, broadcast the signal non_empty, so
	//they try to retake the mutex.
	finish_consumers_exec = 1;
	if (pthread_cond_broadcast(&non_empty) < 0){
		perror("The non_empty condition variable could not be broadcast correctly.\n");
		return -1;
	}
	
	//Release the mutex.
	if (pthread_mutex_unlock(&mutex) < 0){
		perror("ERROR: Mutex unlock could not be correctly executed.\n");
		return -1;
	}
	
	//To recover the partial costs from the consumer threads create 2 variables. The first is an int variable that will
	//contain the sum of all the partial costs, while the second will be a pointer to integer and will be used to receive
	//the returned partial cost from each consumer.
	
	int total_final_cost = 0;
	int* partial_cost;
	for (int i = 0; i < num_consumer; i++){
		if (pthread_join(consumers[i], (void**)&partial_cost) < 0){
			perror("ERROR: The finalization of a consumer thread could not be correctly received.\n");
			return -1;
		}
		total_final_cost += (*partial_cost);
	}
	
	//After performing all the computations, free the dynamic allocated memory.
	free(partial_cost);
	free(type_machine_array);
	free(time_machine_array);
	queue_destroy(circular_queue);
	
	//Also, destroy the mutex and the condition variables as they will not be used anymore.
	if (pthread_mutex_destroy(&mutex) < 0){
		perror("ERROR: Mutex could not be destroyed.\n");
		return -1;
	} 
	if (pthread_cond_destroy(&non_full) < 0){
		perror("ERROR: The non_full condition variable could not be destroyed.\n");
		return -1;
	}
	if (pthread_cond_destroy(&non_empty) < 0){
		perror("ERROR: The non_empty condition variable could not be destroyed.\n");
		return -1;
	}
	if (pthread_cond_destroy(&started) < 0){
		perror("ERROR: The started condition variable could not be destroyed.\n");
		return -1;
	}
	
	//Print in the screen the total cost for performing all these operations.
	printf("Total: %d euros.\n", total_final_cost);
	return 0;
}
