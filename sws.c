/* 
 * CSCI 3120 Operating Systems
 * Group 24
 * Authors:
 *       Gavin Summers B00541980
 *       Ryan 
 *       Jared
 *       Tyler 
 */


/*
 * Global TODO:
 *       - Right now all requests stored in global struct array,
 *         May need to change to Queue data structure as elements will have to
 *         be pulled in and out. 
 *
 *       - Also means that requests will have to be identified in the structure
 *         By their sequence ID instead of array posistion as they are currently 
 *
 *       - Currently storing retrun value from network_open() in structs/arrays as 
 *         fd, this is just an int value though so it doesnt seem to have any meaning
 *          to the client if stored in the array and accessed later
 *
 *       - Currently using sequence (& possibly file descriptor) to identify certain structures
 *         in the linked list. May be a better way to to this
 *
 *       - **** To itterate through the linked list you have a while loop,
 *              You cannot assign to values outside the loop scope, 
 *              Therefore you cannot do something like itterate to find smallest file size
 *
 *       - General cleanup / modularity improvements if possible
 *
 *       - Sequencing algorithms + threads
 *
 *       - Program cannot handle 404 file not found and continue, Causes Seg Fault
 *         Not sure where exactly but probably to do with connections/ports/filestreams
 *         Being open to serve the request, tried to close connections/streams on 404
 *         And leave functions but it didnt work
 *              
 */ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>

#include "network.h"

#define MAX_HTTP_SIZE 8192                 /* size of buffer to allocate */

//global sequence ptr
int *glob_seq;

//request counter variable (not used?)
int *req_count;

//flag to indicate scheduling algorithm
int schedalgo;

//semaphore to wake worker thread
sem_t workerready;

//test mutex
pthread_mutex_t workmutex;

//request structure definition
//    sequence : instructions say to include, indicates what number this job is (eg: 3rd arrived)
//    char buffer: contains relevant info for re-initilizing the request info to send back to client
//    file_descriptor: socket value for the connection the request came from
//    file_size: over all size of file requested by client (eg: meme.txt)
//    size_remain: stars = file_size, should decrement as parts are transfered to client
//    quantum_curr: not used for anything yet, will need for other scheduling algos
typedef struct request{
  int sequence;
  char file_name[1024];
  char buffer[MAX_HTTP_SIZE];
  int file_descriptor;
  int file_size;
  int size_remain;
  int quantum_cur;
} request;

//declaring a pointer to a global linked list node struct
//    This will likely be something that has to be mutex'd
//struct node *n;


//------------------------------------New Q------------------------------
typedef struct Node{
	request req;
	struct Node *next;
} node;

typedef struct Queue{
	struct Node *front;
	struct Node *rear;
	int size;
} queue;

queue *workerqueue;
queue *queue8;
queue *queue64; //Only in MML
queue *queuerr; //Only in MML


int isEmpty(queue *q);

//Initializer
void constructQueues()
{	
	queue8= (queue*)malloc(sizeof(queue));
	queue8 -> size = 0;
 	queue8 -> front = NULL;
	queue8 -> rear = NULL;
	queue64= (queue*)malloc(sizeof(queue));
	queue64 -> size = 0;
 	queue64 -> front = NULL;
	queue64 -> rear = NULL;
	queuerr= (queue*)malloc(sizeof(queue));
	queuerr -> size = 0;
 	queuerr -> front = NULL;
	queuerr -> rear = NULL;
	workerqueue = (queue*)malloc(sizeof(queue));
	queuerr -> size = 0;
 	queuerr -> front = NULL;
	queuerr -> rear = NULL;
	
}

//Adds request to the end of a queue
void enqueue(struct request r, queue *q){
	node *temp = (node*)malloc(sizeof(node));
	(temp -> req) = r;
	(temp -> next) = NULL;
	if (isEmpty(q)){
		(q -> front) = temp;
		(q -> rear) = temp;
	}
	else{
		((q ->rear) -> next) = temp;
		(q -> rear) = temp;	
	}
	(q -> size)++;
	printf("Front-> %d\n Rear -> %d\n", q->front->req.sequence, q->rear->req.sequence);
}

//Removes node at the front of the queue and returns it
node *dequeue(queue *q){

	node *temp;
	if(isEmpty(q)){
		printf("Queue empty");
		return NULL;
	}
	temp = (q -> front);
	(q -> front) = (q -> front) -> next;
	q -> size--;
	temp->next =NULL;
	return temp;
}

//Removes node with given sequence
//ONLY FOR USE WITH SJF, will not work otherwise
//Use dequeue() for RR and MML
node *dequeuePos(int seq){
    node *temp, *prev;
    temp=(queue8->front);
    while(temp!=NULL){
    	if(temp->req.sequence == seq){
        	if(queue8->front->req.sequence==seq){
        		(queue8->front)=queue8->front->next;
        	}
    		else if((temp->next)==NULL){
			queue8->rear= prev;
        		prev->next= NULL;
		}
        	else{
        		prev->next=temp->next;
        	}
		queue8 -> size--;
		temp->next =NULL;
		printf("Dequeueing %d\n", temp->req.sequence); 
		return temp;
    	}
    	else if((temp->next)!=NULL){
        	prev=temp;
        	temp= temp->next;
    	}
	else
	{
		printf("Couldn't find");
		return NULL;
	}
    }
	return NULL;
}

//Checks if queue is empty
int isEmpty(queue *q){
	return ((q->size) == 0);
}

//Prints front and rear of queue
void printQueue(queue *q){
	if(q->front!=NULL && q->rear!=NULL){
		printf("Front-> %d\n Rear -> %d\n", q->front->req.sequence, q->rear->req.sequence);
	}
}

//------------------------------------New Q------------------------------

/*
 * Scheduler in takes in new request struct to add to the linked list
 * Checks if there are bytes left in the job, adds back to queue if there is,
 * Otherwise it says its done and doesnt add it back.
 * 
 * displays current state of the linked list via display call
 * 
 * TODO: Implementation of scheduling algos could either happen here (on enqueing requests)
 *       OR 
 *       could also implement the functionality of the scheduling when requests are
 *       retreived from the linked list in the schedulerOUT function
 * 
 *       Return value should indicate if added successfully or not, 
 *       Currently doesnt really indicate anything
 */
int schedulerIN(struct request newReq){
	//enqueue operation
	if(newReq.size_remain > 0){
		  //number of requests read in so far
		  printf("Glob seq now%d\n\n", *glob_seq);
		if(schedalgo == 0 || schedalgo == 1){
				enqueue(newReq, queue8);
		}
		else if (newReq.quantum_cur == 8){
				enqueue(newReq, queue8);
		}
		else if (newReq.quantum_cur == 64){
				enqueue(newReq, queue64);
		}
		else{
			enqueue(newReq, queuerr);
		}
		return 1;
	}
		else{
			printf("Request done");
			return 0;
		}
}
//-----------------------------------------------------------------------------
/*
 * Scheduler OUT
 * Receives request from main to retrieve next request/job
 * currently Just implents the SJF algo (in a dumb way) 
 * Itterates through all entries in global list and checks size of the request
 *         (change from checking file size to size_remain for other scheduling algos)
 * 
 * TODO: Round Robin Algo
 *       MLFB Algo 
 *       Improved SJF algo
 *       
 *      Will need : update quantums, queue of what jobs in what level of priority
 */
//RETRUNS SEQUENCE ID NUMBER OF SMALLEST SIZE REQUEST IN THE QUEUEUEU

node *schedulerOUT(){
  //notify print out of stuff happening in schedule out
	node *r;  
	if(!isEmpty(queue8)){
		r = queue8->front;
	}
	else if(!isEmpty(queue64)){
		r = queue64 -> front;
	}
	else if(!isEmpty(queuerr)){
		r= queuerr -> front;	
	}
	//SJF determination
	if(schedalgo == 0 && r!= NULL){
		printf("Scheduler OUT start--------------------------------------\n");
	  	//set a local r to point at head of linked list


	  	//set current minsize and sequence id of that linked list entry 
	  	//to the ones in the head of the linked list
	  	int minSize = r->req.file_size;
	  	int smallestID = r->req.sequence;

	  	//print out the above infos
	  	printf("minsize init: %d\n", minSize);
	  	printf("smallest id init: %d\n", smallestID);
	  	//find smallest by itterating through the linked list
	  	//if NULL pointer is reached, print out what the smallest is,
	  	//Otherwise keep going and update the smallest vars if the value in
	  	//the next linked list element is smaller than the current
	  	do{
			printf("queuesize is %d\n", queue8->size);
	  		r=r->next;
	  		if(r == NULL){
				printf("SmallestID: %d\n", smallestID);
				printf("Smallest job is %d bytes long\n", minSize);
	      			printf("Scheduler OUT END------------------------\n");
				return dequeuePos(smallestID);
	    			//return smallestID;
	    		}
	    		else if(r->req.file_size < minSize){
	      			minSize = r->req.file_size;
	      			smallestID = r->req.sequence;
	    		}
	    		//indicate loop occured
	    		printf("looped\n");
	  	}while(r != NULL);

  		//This should not be reached  
  		return dequeue(queue8);//smallestID;
	}
	else if(schedalgo==1 && r!= NULL){
		printQueue(queue8);
		printf("Returning %d\n", queue8->front->req.sequence);
		return dequeue(queue8);
	}
	else if(schedalgo==2 && r!=NULL){
		printf("Queue 1: \n");
		printQueue(queue8);
		printf("Queue 2: \n");
		printQueue(queue64);
		printf("Queue 3: \n");
		printQueue(queuerr);
		if(!isEmpty(queue8)){
			printf("Returning %d\n", queue8->front->req.sequence);
			return dequeue(queue8);
		}
		else if(!isEmpty(queue64)){
			printf("Returning %d\n", queue64->front->req.sequence);
			return dequeue(queue64);
		}
		else{
			printf("Returning %d\n", queuerr->front->req.sequence);
			return dequeue(queuerr);
		}
	}
	else{
		return NULL;
	}
}


//--------------------------------------------------------------------------
/* This function takes a file handle to a client, reads in the request, 
 *    parses the request, and sends back the requested file.  If the
 *    request is improper or the file is not available, the appropriate
 *    error is sent back.
 * Parameters: 
 *             fd : the file descriptor to the client connection (socket)
 * Returns: None
 */
static void request_parse(int fd){
  //buffer holds connection infos for client request
  static char *buffer;
  //req_file becomes parsed file path (eg: /blahblah.txt) client is looking for
  char *req_file = NULL;
  //Request type eg (GET) for client wanting to get page /file from serv
  char *brk;
  char *tmp;
  //This is the file stream used to open a stream from serv to client 
  FILE *fin;
  //will be set to the length of the file to send to the user (maybe can delete)
  int len;
  
  //Allocate mem to buffer of http size
  if( !buffer ) {
    buffer = malloc( MAX_HTTP_SIZE );
    if( !buffer ) {
      perror( "Error while allocating memory");
      abort();
    }
  }
  //0 out the buffer memory to get rid of junk
  memset( buffer, 0, MAX_HTTP_SIZE );

  //try and read the request from the client on fd socket,
  //If it fails throw an error
  if( read( fd, buffer, MAX_HTTP_SIZE ) <= 0 ) {
    perror( "Error while reading request" );
    abort();
  } 
  

  /* standard requests are of the form
   *   GET /foo/bar/qux.html HTTP/1.1
   * We want the second token (the file path).
   * Tokenizes (eg: splits) request into relevent sections
   *  eg: req_file = quz.html
   */
  tmp = strtok_r( buffer, " ", &brk );
  if( tmp && !strcmp( "GET", tmp ) ) {
    req_file = strtok_r( NULL, " ", &brk );
  }

  /* is req valid? */
  if( !req_file ) {                                      
    len = sprintf( buffer, "HTTP/1.1 400 Bad request\n\n" );
    write( fd, buffer, len );
  } 
  //if it is keep going
  else {
    //trims filename (/blah.txt -> blah.txt)
    req_file++;

    //open filestream to the file on the server
    fin = fopen( req_file, "r" );

    //If the filestream to the file fails, throw an error
    if( !fin ) {
      len = sprintf( buffer, "HTTP/1.1 404 File not found\n\n" );  
      write(fd, buffer, len );
    }

    //Request is Good -> check file_size, initilize a structure for it,
    //And then send it to the schedulerIN
    else{
        //Get size of requested file
        fseek(fin, 0, SEEK_END);
        int size = ftell(fin);
        fseek(fin, 0, SEEK_SET);

        //indicate size of file
        printf("In parse: size of file? : %d\n", size);
        
        //allocate new structure for request
        struct request* newReq = malloc(sizeof(struct request));

        //declare structure variables
        newReq->file_size = size;
        newReq->size_remain = size;
        newReq->sequence = *glob_seq;
        newReq->file_descriptor = fd;
		newReq->quantum_cur= 8;
        strcpy(newReq->file_name, req_file);
        strcpy(newReq->buffer, buffer);
        //quantum = ?

        //send request to worker queue 
		enqueue(*newReq, workerqueue);

       
        
        // close the file stream after since we are not reading it 
        // or sending it anymore right now and it is local
        // to this function anyway
        fclose(fin);

    }
  //orig scaffolding closed the socket after, we dont want to do this
  //until the request is full serviced
  //close(fd);
  }
}


//-------------------------------------------------------------------------
/*
 * serve client function
 * Receives index value from main indicating what job to serve next in the array
 * Services the job by transfering the correct amount of data for the quantum
 * 
 */

static void serve_client(){//request *r) {
  //Initilize same stuff needed for sending requested file back to client
  // eg: connection info, socket, file_requested, etc.
  char *buffer; 
  char *req_file = NULL;  
  int fd;
  //char *brk;
  //char *tmp;
  FILE *fin;
  int len;

  //set r to point to head of linked list
	//node *r;
  //r=queue8->front;
	node *r;
	r = schedulerOUT();
/*
    //This itterates through the linked list, looking for
    //The entry with the nextReq sequence ID , 
    //(smallest next request was determined in scheduleOUT and returned to main)
    // Breaks loop when list is found, and pointer r should now point to correct element
    while(r != NULL){
    if(r->req.sequence == nextReq){
      break;
    }
    r=r->next;
  }
  //print out values @ current element r is pointing to (should be same as nextReq)
  printf("r value sequence value: %d\n", r->req.sequence);
  printf("nextreq sequence value: %d\n", nextReq);*/


  //get relevant connection/file infos for this request element
  	fd = r->req.file_descriptor;
  	req_file = r->req.file_name;
  	buffer = r->req.buffer;

    //Do the serving of the request using the above info,
    //open filestream  to correct req_file
    //Check if file is opened correct or not, post status code to client
	
    	fin = fopen( req_file, "r" );     
//if(0){                   /* open file */
    	if( !fin ) {                                    /* check if successful */
     		len = sprintf( buffer, "HTTP/1.1 404 File not found\n\n" );
      		//Try to shut down connection / file reading  
      		write( fd, buffer, len );                     /* if not, send err */
      	} 
	else {                                        /* if so, send file */
        	len = sprintf( buffer, "HTTP/1.1 200 OK\n\n" );/* send success code */
        	write( fd, buffer, len );
	}
	
//}

      //Once file stream opened fine, loop through sending (i think) 1 byte at a time
      //If there is file left to transfer keep going, 
      //If the transfer is @ max transfer allowed 8192 break loop
      //TODO:
      //This functionality will need to be modified to account for quantum read amounts
      //Instead of just transfering all available up to max transfer allowable 
      //Either here or in another function to do other transfer style
	  
	
	printf("About to do something\n");

	if(schedalgo == 0){
		printf("Doing SJF\n");
		fflush(stdout);
		do {                              /* loop, read & send file */
			len = fread( buffer, 1, MAX_HTTP_SIZE, fin ); /* read file chunk */
			if( len < 0 ) {           /* check for errors */
				perror( "Error while writing to client" );
			} 
			else if( len > 0 ) {          /* if none, send chunk */
			  	len = write( fd, buffer, len );
				if( len < 1 ) {          /* check for errors */
				perror( "Error while writing to client" );
				}
			}
		} while( len == MAX_HTTP_SIZE );  /* the last chunk < 8192 */
		
		fclose( fin );
		close(fd);
	}
	else if(schedalgo == 1){
		printf("Doing Round Robin\n");
		fseek(fin, -1*(r->req.size_remain+1), SEEK_END);
		len = fread( buffer, 1, MAX_HTTP_SIZE, fin);
		if( len < 0 ) {                             /* check for errors */
			perror( "Error while writing to client" );
    		} 
		else if( len > 0 ) {                      /* if none, send chunk */
			len = write( fd, buffer, len );
			if( len < 1 ) {           /* check for errors */
            		perror( "Error while writing to client" );
        		}
        	}
		
		//Readd request to queue if it still has more data
	  	r->req.size_remain -= len;
		printf("Total size: %d", r->req.file_size);
		printf("Remaining size: %d", r->req.size_remain);
		if(r->req.size_remain<0){
      			printf("Request %d completed\n", r->req.sequence);
			free(r);
			fclose(fin);
			fflush(stdout);		
			close(fd);
		}
		else{
			printf("Still some left\n");
			fflush(stdout);
			schedulerIN(r->req);
			fclose(fin);
		}
      	}
	else if(schedalgo == 2){
		printf("Doing multilevel\n");
		int i= 0;
		if(r->req.quantum_cur == 8){
			i=8;
		}
		printf("i: %d", i); 

			fseek(fin, -1*(r->req.size_remain+1), SEEK_END);
		do{
			len = fread( buffer, 1, MAX_HTTP_SIZE, fin);
			if( len < 0 ) {         /* check for errors */
				perror( "Error while writing to client" );
	    		} 
			else if( len > 0 ) {         /* if none, send chunk */
				len = write( fd, buffer, len );
				if( len < 1 ) {           /* check for errors */
		    			perror( "Error while writing to client" );
				}
			}
	  		r->req.size_remain -= len;
			i++;
		}while(len == MAX_HTTP_SIZE && i<8);
		
		//Readd request to queue if it still has more data
	  	r->req.size_remain -= len;
		printf("Total size: %d", r->req.file_size);
		printf("Remaining size: %d", r->req.size_remain);
		if(r->req.size_remain<0){
      			printf("Request %d completed\n", r->req.sequence);
			free(r);
			fclose(fin);
			fflush(stdout);		
			close(fd);
		}
		else{
			printf("Still some left\n");
			fflush(stdout);
			if(r->req.quantum_cur==8){
				printf("Moving to 64 queue\n");
				r->req.quantum_cur=64;
			}
			else if(r->req.quantum_cur==64){
				printf("Moving to RR queue\n");
				r->req.quantum_cur=128; //Not actually 128, just a marker
			}
			schedulerIN(r->req);
			fclose(fin);
		}
	}
	 

    //delete served request from list once complete
    //*** Remove this out in similar implementation if doing quantum based transfers****
  //Close connection to client socket when done
  //*** Remove this out in similar implementation if doing quantum based transfers****
  //close( fd );
}

void worker(){
	while(1){
		if(!isEmpty(workerqueue)){
			//Dequeue Operation
			struct request newReq = dequeue(workerqueue)->req;
			//schedule the request
			schedulerIN(newReq);
	  	} 
		else{
			sem_wait(&workerready);
			if(!isEmpty(queue8) || !isEmpty(queue64) || !isEmpty(queuerr)){
				printf("\nThere is still so much to do!\n");	
				//int nextReq = schedulerOUT();
				pthread_mutex_lock(&workmutex);
				serve_client();//nextReq);
				printf("Queue size: %d\n", queue8->size);
				pthread_mutex_unlock(&workmutex);
			}
		}
	}
}


//--------------------------------------------------------------------------
/* This function is where the program starts running.
 *    The function first parses its command line parameters to determine port #
 *    Then, it initializes, the network and enters the main loop.
 *    The main loop waits for a client (1 or more to connect, and then processes
 *    all clients by calling the seve_client() function for each one.
 * Parameters: 
 *             argc : number of command line parameters (including program name
 *             argv : array of pointers to command line parameters
 * Returns: an integer status code, 0 for success, something else for error.
 */
int main( int argc, char **argv ) {
  int port = -1;
  //init number of requests so far to one
  int globe_count = 1;
  int fd;
  int workernum;

  //initialize semaphore
  sem_init(&workerready, 0, 0);
  //initialize mutex
  if (pthread_mutex_init(&workmutex, NULL) != 0)
  {
    printf("\n mutex initilization failed\n");
    return 1;
  }




  /* check cmd line arguments, will be used to determine schedule algos later
   */
  //Has port info ?
  if( ( argc < 4 ) || ( sscanf( argv[1], "%d", &port ) < 1 ) ) {
    printf( "usage: sms <port> <scheduling algorithm> <# of worker threads>\n" );
    return 0;
  }

  //What schedule 
  if(strcmp(argv[2], "SJF") == 0){
    printf("Shortest Job first Scheduling\n");
	schedalgo=0;
  }
  else if(strcmp(argv[2], "RR") == 0){
    printf("Round Robin Scheduling!\n");
	schedalgo=1;
  }
  else if (strcmp(argv[2], "MLFB") == 0){
    printf("Multi level Feedback queue !\n");
	schedalgo=2;
  }
  else{
    printf("cannot determine scheduling algo, terminating\n");
    return 0;
  }
	
  //set number of worker threads
  workernum = atoi(argv[3]);

  constructQueues();	
  //point the global sequence to the globe_count
  glob_seq = &globe_count;

  //initilize network & port stuffs
  network_init( port );

  //assign specified amount of worker threads
  pthread_t *workers;
  if ( (workers  = malloc(sizeof(pthread_t)*workernum)) == NULL){
    printf("Error Allocating Memory, exiting...\n");
    return -1;
  }

  //create the threads
 // for(int i = 0; i < workernum; ++i){
 //   pthread_create(&workers[i], NULL, (void *)&worker, NULL);
 // }

  //infinite loop 
  for( ;; ) {
    //wait for new requests
    network_wait();

    // if a request is recieved, open a socket and set it to fd,
    // use the fd to get the request info, parse will send it to schedule_IN
    // then set the int next_req to the sequence ID of a request found
    // by scheduler in (currently it finds using the smallest file_size val)
    // Then serve the request to the client

    for( fd = network_open(); fd >= 0; fd = network_open() ) {
	
      request_parse(fd);
  	//int nextReq = schedulerOUT();
	//serve_client();//nextReq);
	//printf("queue size: %d\n", queue8->size);
    }
	
	while(!isEmpty(queue8) || !isEmpty(queue64) || !isEmpty(queuerr)){
		sem_post(&workerready);
		//serve_client();
	}
	//worker();

  }
  //destructQueues();//Will probably never hit this
}
