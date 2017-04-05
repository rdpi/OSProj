## OS Project

to run...
    Job (1)
        make
        ./sws port algo
        (./sws 30808 SJF)
    Job (2)
        Either visit localhost:30808/file.txt
        or
        ./hydra < test1.in


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
 */ # OSProj
# OSProj
