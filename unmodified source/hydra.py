#!/usr/bin/python


#######################################
#
#  Hydra is a tool for testing your webserver
# 
#  Hydra reads in a test script from stdin, i.e. to run hydra use the command:
#
#  ./hydra.py < test.in
#
# where test.in is a file containing a test script.  The format of the test
# file is as follows:
# - The first line of the file is a single integer, denoting the port number
#   of the webserver
# - The remain lines represent requests to the server.  Each request comprises
#   three parts:
#   + delay : number of seconds (float) before connecting to the server
#   + pause : number of seconds (flaot) after connectkng before sending request
#   + file  : file (path) to requet
#   Here is an example of a test file that makes hydra connect to the webserver
#   on port 8080 and requests four files:
#
#   8080
#   0.0 1.0 sws.c
#   0.5 0.0 network.c
#   0.5 0.0 network.h
#   0.5 0.0 makefile
#
# The first request connects immediately, but waits for 1 second before sending
# the request.  The remaining requests all wait 0.5 seconds before connecting.
# By setting the delay and the pause appropriate we can control how many clients
# are connected to the webserver at any time.
#
# If the -t swictch is used, e.g.,
#
#  ./hydra.py -t < test.in
#
# the times of the response are printed instead of the output.


import threading, sys, socket, time, string, Queue, os

port = int( sys.stdin.readline() );
num = 0
finished = 0
dotime = len( sys.argv ) > 1 and sys.argv[1] == "-t"
completed = Queue.Queue()

class DeadMan(threading.Thread):
  def __init__(self, wait):
    threading.Thread.__init__(self)
    self.wait = wait

  def run(self):
    while finished == 0:
      time.sleep( 0.1 )

    while self.wait > 0:
      if finished == num + 1:
       return
      time.sleep( 0.1 )
      self.wait = self.wait - 0.1

    print "Harikari"
    os._exit(0)

class RequestHead(threading.Thread):
  def __init__(self, delay, pause, file):
    global num
    threading.Thread.__init__(self)

    self.delay = delay
    self.pause = pause
    self.file = file
    num = num + 1
    self.seq = num

  def run(self):
    global finished

    if self.delay > 0:
      time.sleep( self.delay )

    sock = socket.socket();
    try: 
      sock.connect( ("localhost", port) );
    except:
      print "Unable to connect"
      finished = finished + 1
      return

    if self.pause > 0:
      time.sleep( self.pause )

    try: 
      req = "GET /" + self.file + " HTTP/1.1\nHost: localhost\n\n" 
      req_time = time.time()
      sock.sendall( req )
      sock.shutdown( socket.SHUT_WR )

      result = ""
      resp = sock.recv( 8192 )
      while len( resp ) > 0:
        result = result + resp
        resp = sock.recv( 8192 )
      completed.put( self.seq )
      result = result + resp
      req_time = time.time() - req_time
      sock.close()
    except:
      print "Socket failure"
      finished = finished + 1
      return

    while finished != self.seq:
      time.sleep( 0.01 )
    print "Thread " + str( self.seq ) + \
          " ============================================"
    if dotime:
      print "{:10.7f}".format( req_time ) + " seconds"
    else:
      print result

    finished = finished + 1

      
try:
  for line in sys.stdin.readlines():
    req = string.split( string.strip( line ), " " )

    if len( req ) > 2:
      RequestHead( float( req[0] ), float( req[1] ), req[2] ).start()

  DeadMan(10.0).start()
except:
  print "Out of threads, asta la vista"
  os._exit(0)

finished = 1
while finished != num + 1:
  time.sleep( 0.01 )

print "====================================================================="
print "Request completion order: ",
while not completed.empty():
  print str( completed.get() ) + " ",
print
