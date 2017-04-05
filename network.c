/* 
 * File: network.c
 * Author: Alex Brodsky
 * Purpose: This file contains the network module to accept web connections.
 *          Please see network.h for documentation on how to use this module.
 */


#include <stddef.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <poll.h>

#include "network.h"

static int serv_sock = -1;

/* This function checks if there are any web clients waiting to connect.
 *    If one or more clients are waiting to connect, this function returns.
 *    Otherwise, this function puts the program to sleep (blocks) until
 *    a client connects.
 * Parameters: None
 * Returns: None
 */
extern void network_wait() {
  int n;                                                /* result var */
  fd_set sel;                                           /* descriptor bits */
  fd_set err;
  
  if( serv_sock < 0 ) {                                 /* sanity check */
    perror( "Error, network not initalized" );
    abort();
  }

  FD_ZERO( &sel );                                      /* initialize bits */
  FD_ZERO( &err );
  FD_SET( serv_sock, &sel );
  FD_SET( serv_sock, &err );

  n = select( serv_sock + 1, &sel, NULL, &err, NULL );  /* wait for conn. */

  if( ( n <= 0 ) || FD_ISSET( serv_sock, &err ) ) {     /* check for errors */
    perror( "Error occurred while waiting" );
    abort();
  } 
}


/* This function checks if there are any web clients waiting to connect.
 *    If one or more clients are waiting to connect, this function opens
 *    a connection to the next client waiting to connect, and returns an
 *    integer file descriptor for the connection.  If no clients are 
 *    waiting, this function returns -1.
 * Parameters: None
 * Returns: A positive integer file decriptor to the next clients connection,
 *          or -1 if no client is waiting.
 */
extern int network_open() {
  struct sockaddr_in server;                            /* addr of client */
  int len = sizeof( server );                           /* length of addr */
  int n;                                                /* return var */
  int sock = -1;                                        /* socket for client */
  fd_set sel;                                           /* descriptor bits */
  fd_set err;
  struct timeval tv;                                    /* time to wait */
  
  if( serv_sock < 0 ) {                                 /* sanity check */
    perror( "Error, network not initalized" );
    abort();
  }

  FD_ZERO( &sel );                                      /* check for client */
  FD_ZERO( &err );
  FD_SET( serv_sock, &sel );
  FD_SET( serv_sock, &err );
  memset( &tv, 0, sizeof( tv ) );
  n = select( serv_sock + 1, &sel, NULL, &err, &tv );

  if( ( n < 0 ) || FD_ISSET( serv_sock, &err ) ) {      /* check for errors */
    perror( "Error occurred on select()" );
    abort();
  } else if( ( n > 0 ) && FD_ISSET( serv_sock, &sel ) ) { /* client is waiting*/
    /* get client connection */
    sock = accept( serv_sock, (struct sockaddr *)&server, (socklen_t *)&len );

    if( sock < 0 ) {                                    /* check for errors */
      perror( "Error occurred on select()" );
    }
  }
  return sock;                                          /* return client conn.*/
}


/* This function initializes the network module and creates a server socket
 *   bound to a specified port.  This function will abort the program if an
 *   error occurs.
 * Parameters: 
 *             port : the port on which the server should listen.  Should be
 *                    between 1024 and 65525
 * Returns: None
 */
extern void network_init( int port ) {
  struct sockaddr_in self;                             /* socket address */
  int yes = 1;                                         /* config variable */
  
  serv_sock = socket( PF_INET, SOCK_STREAM, 0 );       /* create socket */
  if( serv_sock < 0 ) {
    perror( "Error while creating server socket" );
    abort();
  } 

                                                       /* configure socket */
  setsockopt( serv_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof( int ) );
  setsockopt( serv_sock, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof( int ) );

  self.sin_family = AF_INET;                           /* bind socket to port */
  self.sin_addr.s_addr = htonl( INADDR_ANY );
  self.sin_port = htons( port );
  if( bind( serv_sock, (struct sockaddr *)&self, sizeof( self ) ) )  {
    perror( "Error on bind()" );
    abort();
  }

  if( listen( serv_sock, 64 ) ) {                      /* allow connections */
    perror( "Error on listen()" );
    abort();
  }
}





