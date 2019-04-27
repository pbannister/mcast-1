# mcast-1

Example/test of multicast. 
In the examples below 192.168.86.248 and 192.168.86.20 are test system addresses.
Note the differences between the unicast and multicast examples are minor.

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
</pre>

### Test #1 - server-like usage with multicast.
From 192.168.86.248:
<pre>bin/mcast-1 -b 0:12345 -m 234.1.2.3@192.168.86.248 -1</pre>

This test:
*  Binds the socket to the endpoint 0:12345.
*  Specifies the interface 192.168.86.248 as multicast.
*  For the interface adds multicast membership to the group 234.1.2.3.
*  Receives (from any sender) and replies to caller.

Note that in the above setup, the service will receive on port 12345
multicast and direct UDP messages through <b>any</b> network interface.
If this is what you want, the above form is sufficient.
In more complex network setups, you might want to ensure that client messages 
can only come in through a <i>specific</i> network interface.
You can specifiy the network interface by changing the socket bind():
<pre>bin/mcast-1 -b 192.168.86.20:12345 -m 234.1.2.3@192.168.86.248 -1</pre>

### Test #1 - server-like usage with ordinary UDP.
From 192.168.86.248:
<pre>bin/mcast-1 -b 192.168.86.248:12345 -1</pre>

This test:
*  Binds the socket to the endpoint 192.168.86.248:12345.
*  Receives (from any sender) and replies to caller.

### Test #2 - client-like usage with multicast.
From 192.168.86.20:
<pre>bin/mcast-1 -d 234.1.2.3:12345 -i 192.168.86.20 -2</pre>

This test:
*  Specifies the destination (a multicast address).
*  Sends to the destination, and receives any reply.

Note that specifying the multicast interface (the "-i" option) is not always needed. 
Sometimes your system will automatically pick and configure the right network interface.
Sometimes you may have another application that has enabled multicast on the needed network interface.

But ... if not explicitly specified, sometimes the proper interface will *not* enabled for multicast.
This can prove ... *very* confusing. 
Better to always explicitly indicate the multicast network interface.
 
### Test #3 - client-like usage with ordinary UDP.
From 192.168.86.20:
<pre>
bin/mcast-1 -d 192.168.86.20:12345 -2
</pre>

This test:
*  Specifies the destination (a unicast address).
*  Sends to the destination, and receives any reply.

## Use-cases

My primary use-case in creating this tool was to diagnose use of multicast.
This tool is also useful in discovering firewall-induced problems.
(In my case the default Centos 7 firewall was the source of grief.)

Note that if you are well-acquainted with TCP client/server network usage
(as am I *very* acquainted)
the world of UDP is a bit wild and wooly. 
There is scarce reason to follow the same patterns.
You can use (and mis-use) UDP in very different pattern.
So I have place "client" and "server" in quotes, to remind this is not a fixed pattern.

### 1) Verify ordinary unicast UDP.

Given two computers at (for example) addresses 192.168.86.20 (the "service" side),
and address 192.168.86.248 (the "client" side), a test we can perform:

First, start the "server" on 192.168.86.20 port 12345:
<pre>bin/mcast-1 -b 192.168.86.20:12345 -1</pre>

Then start the "client" on 192.168.86.248:
<pre>bin/mcast-1 -d 192.168.86.20:12345 -2</pre>

Assuming no firewalls, and a proper network routes between the two systems, this will just work.
If this does not work, completely disable any firewalls, and reboot. 

(**Never** do this when connected directly to the internet.)

If this test does not work, you have generic network issues to resolve.

If this test fails with your firewall, and works without, then you have firewall issues.

### 2) Verify client unicast to a multicast service.

Assuming you have done the prior unicast test, with success, then try multicast.
Well not *exactly*. 
We can cheat a bit.
Try a "server" configured for multicast, but use a unicast client:

Start a proper multicast receiving "service" side on 192.168.86.20 with:
<pre>bin/mcast-1 -b 0:12345 -m 234.1.2.3@192.168.86.20 -1</pre>

Where a "client" sending to 234.1.2.3:12345 is expected to be received by *all* the services listening on the multicast address 234.1.2.3:12345. 

But in this test, to be sure the service side is properly configured for multicast, we use a unicast client.
<pre>bin/mcast-1 -d 192.168.86.20:12345 -2</pre>

This step should just work. If not then you missed something prior.
If this step works, you are ready to debug any client multicast issues.
(Expect to encounter and debug some of your firewall issues at this point.)

### 3) Verify client multicast to a multicast service.

This is, of course, where we were aiming from the start. :)

With the "service" from prior exercise, start a proper multicast client with:
<pre>bin/mcast-1 -d 234.1.2.3:12345 -i 192.168.86.248 -2</pre>

Note that you can leave out explicitly specifying the multcast interface (the "-i" option).
Sometimes you system will pick the right interface. Sometimes not. This can be confusing.

Always explicitly specify the correct multicast interface.
At the code level, this is logically <pre>setsockopt(,IP_MULTICAST_IF,"192.168.86.248")</pre> in the example.

Without a firewall, this example should just work.
With a firewall, in my case, it did not. 
Needed to diagnose the firewall induced problems, at this point.

## Diagnostics for the multicast service.

In another window, you monitor something of the Linux networking status specific to multicast with a bit of script:
<pre>while clear ; do ip maddress show eth1 ; cat /proc/net/igmp ; sleep 2 ; done</pre>

This presumes that <i>eth1</i> is the network interface meant for multicast use.

## Centos 7 *firewalld* specific guidance.

In my specific present case, the Centos 7 *firewalld* was the source of grief.
Of certain, I do not claim to do be an expert on *firewalld* or intended function.
What I can do is describe what worked for my problem.

To get multicast working, I needed to tell *firewalld*:
<pre>
firewall-cmd --permanent --add-protocol igmp
firewall-cmd --permanent --add-port 12345/udp
firewall-cmd --permanent --add-source-port 12345/udp
firewall-cmd --reload
</pre>

The resulting firewalld configuration on my recently installed Centos 7 test system was:
<pre># cat /etc/firewalld/zones/public.xml
&lt;?xml version="1.0" encoding="utf-8"?>
&lt;zone>
  &lt;short>Public&lt;/short>
  &lt;description>For use in public areas. You do not trust the other computers on networks to not harm your computer. Only selected incoming connections are accepted.&lt;/description>
  &lt;service name="ssh"/>
  &lt;service name="dhcpv6-client"/>
  &lt;port protocol="udp" port="12349"/>
  &lt;source-port protocol="udp" port="12349"/>
&lt;/zone>
</pre>
You might want this for other than the "public" zone, in less usual usage.

This worked for me. 
Clarification welcomed.
