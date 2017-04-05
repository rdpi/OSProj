/* 
 * File: network.h
 * Author: Alex Brodsky
 * Purpose: This file contains the prototypes and describes how to use network
 *          module to accept web connections.
 */

#ifndef NETWORK_H
#define NETWORK_H

#include <stdio.h>

/* 
 * This module has three functions:
 *   network_init() : inititalizes the module
 *   network_wait() : wait until a client connects
 *   network_open() : open the next client connection
 *
 * The network_init() function should be called once, at the start of the 
 * program.  This function will create a socket to which web clients can 
 * connect.
 *
 * The network_wait() function should be called when there are no more web
 * clients waiting to connect.  This function will put the program to sleep 
 * until one or more web clients connects.  Once one or web client connect,
 * this function returns and your program can accept the connections.
 *
 * The network_open() function opens a waiting web client connection and
 * returns an integer file descriptor.  If no clients are waiting, this 
 * function returns -1.
 */


/* This function initializes the network module and creates a server socket
 *   bound to a specified port.  This function will abort the program if an
 *   error occurs.
 * Parameters: 
 *             port : the port on which the server should listen.  Should be
 *                    between 1024 and 65525
 * Returns: None
 */
extern void network_init( int port );


/* This function checks if there are any web clients waiting to connect.
 *    If one or more clients are waiting to connect, this function returns.
 *    Otherwise, this function puts the program to sleep (blocks) until
 *    a client connects.
 * Parameters: None
 * Returns: None
 */
extern void network_wait();


/* This function checks if there are any web clients waiting to connect.
 *    If one or more clients are waiting to connect, this function opens
 *    a connection to the next client waiting to connect, and returns an
 *    integer file descriptor for the connection.  If no clients are 
 *    waiting, this function returns -1.
 * Parameters: None
 * Returns: A positive integer file decriptor to the next clients connection,
 *          or -1 if no client is waiting.
 */
extern int network_open();

#endif
