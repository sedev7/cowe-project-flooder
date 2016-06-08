#!/usr/bin/perl
# 
# flooderSocket.pl
#
# Flooder Program for remote control of packet script.
#
# v4:
#  - Clean out old code.
#  - Figured out problem with connection - need iptables rule.
#  - Create start file from message received and put in 'listen' directory.
#  - Changed name from flooder1.pl to flooderSocket.pl.
#
# v3:
#  - Clean out old code.
#  - Create start file to send to flooderpgrm.
#
# v2:
#  - Attempt to start udpinject script.
#

use strict;
use IO::Socket;

my $server_port = 8080;
my $start_file = "/home/jim/cowe/listen/start";
my $stop_file = "/home/jim/cowe/listen/stop";

# Make the socket
my $local = IO::Socket::INET->new(
	Proto 	=> 'tcp',
        LocalPort => $server_port,
	LocalAddr=> '0.0.0.0',
	Reuse	=> 1
) or die "$!";

$local->listen();
$local->autoflush(1);

print "SERVER started on port $server_port\n";
print "At your service.  Waiting...\n";
  
# Accept and process connections
my $addr;	# Client handle
while ( $addr = $local->accept() ) {
	
	print "Connected from: ", $addr->peerhost();
	print " Port: ", $addr->peerport(), "\n";

	my @result;
	my $i = 0;
        my $cmd = "";
        my $runtime = 0;
        #my $pid = 0;
        my $recdStart = 0;
        my $recdStop = 0;

	while (<$addr>) {	# Read all messages from client
	  last if m/^end/gi;	# Exit loop if 'end' message

	  $result[$i] = $_;
	  if ($result[$i] =~ /^start.*/) {
            $cmd = "start";
            print "received $cmd\n";
            # Return message to client
            print $addr "ACK-Received $cmd\n";
            $recdStart = 1;

          } elsif ($recdStart && $result[$i] =~ /^\d+|.*/) {
            $cmd = "start";
            $runtime = $result[$i];
	    print "received start file message:  $runtime \n";
            print $addr "ACK-Received start file\n";
            ### Create a start file to start the flooder
            ### Parse the message sent over the socket
            ### Format: [ start|<sourc_ip>|<dest_ip>|<source_portno>|dest_portno>|<time_interval> ]
            #unless(open FILE, ">$start_file") {
            #  die "Unable to create start file\n";
            #}
            open(FILE, ">$start_file") || warn "Unable to create 'start' file\n";
            #print FILE "$result[$i]\\0";
            print FILE "$result[$i]";
            close FILE;

            # Reset the flag
            $recdStart = 0;

          //} elsif ($recdStart && $result[$i] =~ /^stop/) {
          } elsif ($result[$i] =~ /^stop/) {
            $cmd = "stop";
            $recdStop = 1;
            $runtime = $result[$i];
	    print "received stop message:  $runtime \n";
            print $addr "ACK-Received $cmd message\n";
            ### Create a stop file to stop the flooder
            #unless(open FILE, ">$stop_file") {
            #  die "Unable to create stop file\n";
            #}
            open(FILE, ">$stop_file") || warn "Unable to create 'stop' file\n";
            print FILE "stop";
            close FILE;

            # Reset the flag
            $recdStop = 0;

          } else {
            if ($cmd ne "start" || $cmd ne "stop") {
	      print "nice try!\n";
              print $addr "ACK-NG\n";
            }
	  }
	  $i++;
	}
	chomp;
	if (m/^end/gi) {	# Needed to prevent client termination
	  my $send = "ACK-Received end of socket message";
	  print $addr "$send\n";
	}
	print "Closed connection\n";
	close $addr;

	print "At your service.  Waiting...\n";

   # Wait for next request
}

close(SERVER);
