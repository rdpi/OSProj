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

#include "network.h"

#define MAX_HTTP_SIZE 8192                 /* size of buffer to allocate */

//global sequence ptr
int *glob_seq;

//request counter variable (not used?)
int *req_count;

int schedalgo;


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


//linked list for storing requests
//    Above requests structs stored in linked list,
//    request req = data part,
//    node *next = pointer to next element in chain
struct node{
  request req;
  struct node *next;
}*head;

//declaring a pointer to a global linked list node struct
//    This will likely be something that has to be mutex'd
//struct node *n;


//**********************************Bunch of Linked list functions,
// TODO: Change struct name from num to something more descriptive in all
//       Move to seperate file if possible
void append(struct request num)
{
    struct node *temp,*right;
    temp= (struct node *)malloc(sizeof(struct node));
    temp->req=num;
    right=(struct node *)head;
    while(right->next != NULL)
    right=right->next;
    right->next =temp;
    right=temp;
    right->next=NULL;
}
 
void add(struct request num )
{
    struct node *temp;
    temp=(struct node *)malloc(sizeof(struct node));
    temp->req=num;
    if (head== NULL)
    {
    head=temp;
    head->next=NULL;
    }
    else
    {
    temp->next=head;
    head=temp;
    }
}

void addafter(struct request num, int loc)
{
    int i;
    struct node *temp,*left,*right;
    right=head;
    for(i=1;i<loc;i++)
    {
    left=right;
    right=right->next;
    }
    temp=(struct node *)malloc(sizeof(struct node));
    temp->req=num;
    left->next=temp;
    left=temp;
    left->next=right;
    return;
}

int count()
{
    struct node *n;
    int c=0;
    n=head;
    while(n!=NULL)
    {
    n=n->next;
    c++;
    }
    return c;
}
 
 void insert(struct request num)
{
    int c=0;
    struct node *temp;
    temp=head;
    if(temp==NULL)
    {
    add(num);
    }
    else
    {
    while(temp!=NULL)
    {
        if(temp->req.file_size < num.file_size)
        c++;
        temp=temp->next;
    }
    if(c==0)
        add(num);
    else if(c<count())
        addafter(num,++c);
    else
        append(num);
    }
}
 
int delete(struct request num)
{
    struct node *temp, *prev;
    temp=head;
    while(temp!=NULL)
    {
    if(temp->req.sequence == num.sequence)
    {
        if(temp==head)
        {
        head=temp->next;
        free(temp);
        return 1;
        }
        else
        {
        prev->next=temp->next;
        free(temp);
        return 1;
        }
    }
    else
    {
        prev=temp;
        temp= temp->next;
    }
    }
    return 0;
}

void  display(struct node *r)
{
    printf("******Display linked list Contents*****\n");
    r=head;
    if(r==NULL)
    {
    return;
    }
    while(r!=NULL)
    {
      printf("sequenceid: %d\n",r->req.sequence);
      printf("filename: %s\n",r->req.file_name);
      printf("file_descriptor: %d\n", r->req.file_descriptor);
      printf("file_size: %d\n", r->req.file_size);
      printf("data_remaining: %d\n", r->req.size_remain);
      printf("-----------------------------------------\n");


      r=r->next;
    }
    printf("****** END Display linked list Contents*****\n");
    printf("\n");
}
//************************************END OF LINKED LIST FUNCTIONS



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
  //Enqueue Operation
    if(newReq.size_remain > 0){

      //number of requests read in so far
      printf("Glob seq now%d\n\n", *glob_seq);

      insert(newReq);
      display(head);
      return 0;
    }
    else{
      printf("Request %d completed\n", newReq.sequence);
      return 0;
    }
  return 0; 
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

int schedulerOUT(struct node *r){
  //notify print out of stuff happening in schedule out
  printf("Scheduler OUT start--------------------------------------\n");
  //set a local r to point at head of linked list
  r = head;

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
    if(r == NULL){
      printf("SmallestID: %d\n", smallestID);
      printf("Next smallest job is %d bytes long\n", smallestID);
      printf("Scheduler OUT END------------------------------------------\n");
      return smallestID;
    }
    else if(r->req.file_size < minSize){
      minSize = r->req.file_size;
      smallestID = r->req.sequence;
    }
    r=r->next;
    //indicate loop occured
    printf("looped\n");
  }while(r != NULL);

  //This should not be reached  
  return smallestID;
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
    //Leave function if bad request
    abort();
    return;
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
      //leave function if file not found
      //Closing filestream and socket (not working?)
      fclose(fin);
      close(fd);
      abort();
      return;
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
        strcpy(newReq->file_name, req_file);
        strcpy(newReq->buffer, buffer);
        //quantum = ?

        //send request struct to schedulerIN, 
        int result = schedulerIN(*newReq);
        
        //Supposed to check if the request was successfully added, 
        //ScheduleIN in always returns 0 right now though
        if(result == 0){
          len = sprintf( buffer, "HTTP/1.1 200 OK\n\n" );
          *glob_seq = *glob_seq + 1;
          printf("Global seq: %d\n\n", *glob_seq);
          write( fd, buffer, len );
        }
        else{
          printf("Error with scheduling request\n");
        }        
        
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

static void serve_client(int nextReq, struct node *r) {
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
  r=head;

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
  printf("nextreq sequence value: %d\n", nextReq);


  //get relevant connection/file infos for this request element
  fd = r->req.file_descriptor;
  req_file = r->req.file_name;
  buffer = r->req.buffer;

    //Do the serving of the request using the above info,
    //open filestream  to correct req_file
    //Check if file is opened correct or not, post status code to client
    fin = fopen( req_file, "r" );                        /* open file */
    if( !fin ) {                                    /* check if successful */
      len = sprintf( buffer, "HTTP/1.1 404 File not found\n\n" );
      //Try to shut down connection / file reading  
      write( fd, buffer, len );                     /* if not, send err */
      fclose(fin);
      close(fd);
      //Try to exit function, still causes segmentation fault 
      //currently
      return;
      } else {                                        /* if so, send file */
        len = sprintf( buffer, "HTTP/1.1 200 OK\n\n" );/* send success code */
        write( fd, buffer, len );

      //Once file stream opened fine, loop through sending (i think) 1 byte at a time
      //If there is file left to transfer keep going, 
      //If the transfer is @ max transfer allowed 8192 break loop
      //TODO:
      //This functionality will need to be modified to account for quantum read amounts
      //Instead of just transfering all available up to max transfer allowable 
      //Either here or in another function to do other transfer style
	  
	
	printf("About to do something\n");
      if(schedalgo == 1){
		 printf("Doing Round Robin\n");
		fseek(fin, -1*(r->req.size_remain+1), SEEK_END);
		len = fread( buffer, 1, MAX_HTTP_SIZE, fin);
		if( len < 0 ) {                             /* check for errors */
            perror( "Error while writing to client" );
        } else if( len > 0 ) {                      /* if none, send chunk */
          len = write( fd, buffer, len );
          if( len < 1 ) {                           /* check for errors */
            perror( "Error while writing to client" );
          }
        }
		request remainreq;
		//Make a new request and add it back into the queue
  		strcpy(remainreq.file_name, r->req.file_name);
  		strcpy(remainreq.buffer, r->req.buffer);
   		remainreq.file_descriptor = r->req.file_descriptor;
  		remainreq.file_size = r->req.file_size;
  		r->req.size_remain -= len;
  		remainreq.quantum_cur = r->req.quantum_cur;

		delete(r->req);
		schedulerIN(remainreq);
		display(head);
      }
	  if(schedalgo == 0){
		  printf("Doing SJF\n");
		  do {                                          /* loop, read & send file */
			len = fread( buffer, 1, MAX_HTTP_SIZE, fin );  /* read file chunk */
			if( len < 0 ) {                             /* check for errors */
				perror( "Error while writing to client" );
			} else if( len > 0 ) {                      /* if none, send chunk */
			  len = write( fd, buffer, len );
			  if( len < 1 ) {                           /* check for errors */
				perror( "Error while writing to client" );
			  }
			}
		  } while( len == MAX_HTTP_SIZE );              /* the last chunk < 8192 */
		  fclose( fin );
		}
	 }

    //delete served request from list once complete
    //*** Remove this out in similar implementation if doing quantum based transfers****
    delete(r->req);
  //Close connection to client socket when done
  //*** Remove this out in similar implementation if doing quantum based transfers****
  close( fd );
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

  /* check cmd line arguments, will be used to determine schedule algos later
   */
  //Has port info ?
  if( ( argc < 2 ) || ( sscanf( argv[1], "%d", &port ) < 1 ) ) {
    printf( "usage: sms <port>\n" );
    return 0;
  }
  //Indicates shceduling algo to use?
  if(argc < 3){
    printf("what scheduling algo?\n");
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

  //point the global sequence to the globe_count
  glob_seq = &globe_count;

  //initilize network & port stuffs
  network_init( port );

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
  		int nextReq = schedulerOUT(head);
		serve_client(nextReq,head);
    }
	printf("Count: %d", count());
	while(count(head) != 0){
		int nextReq = schedulerOUT(head);
		serve_client(head->req.sequence,head);
	}
  }
}
