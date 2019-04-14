# mcast-1

Example/test of multicast.

## Usage
<pre>
$ bin/mcast-1 -h

Usage:
	bin/mcast-1 [ options ]

Where options are:
	-b endpoint      bind to endpoint  : 
	-d endpoint      send to endpoint  : 
	-i address       mcast interface   : 
	-m address       mcast membership  : 
	-1               test 1
	-2               test 2
	-3               test 3
</pre>

## Test #1 - server-like usage.
<pre>
bin/mcast-1 -b 0:12345 -i 192.168.86.248 -m 234.1.2.3 -1
</pre>

This test:
*  Binds the socket to the endpoint 0:12345.
*  Specifies the interface 192.168.86.248 as multicast.
*  For the interface adds multicast membership to the group 234.1.2.3.
*  Receives (from any sender) and replies to caller.

## Test #2 - client-like usage.
<pre>
bin/mcast-1 -b 192.168.86.20:1234 -d 234.1.2.3:12345 -2
</pre>

This test:
*  Binds the socket to the endpoint 192.168.86.20:1234.
*  Specifies the destination (as a multicast address).
*  Sends to the destination, and receives any reply.
 
