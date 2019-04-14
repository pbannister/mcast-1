# mcast-1

Example/test of multicast.

# Example server-like usage.

<pre>
bin/mcast-1 -b 0:12345 -i 192.168.86.248 -m 234.1.2.3 -1
</pre>

This binds the socket to the endpoint 0:12345, 
with the interface 192.168.86.248 specified as multicast, 
and the interface added multicast membership to 234.1.2.3.
Test #1 receives (from any sender) and replies to caller.

 
