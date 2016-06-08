#!/usr/bin/perl
# 
# flooder1.pl
#
# Flooder Program for remote control of packet script.
#

use strict;
#use Socket;
use IO::Socket;

my $server_port = 8080;

# Make the socket
#socket(SERVER, PF_INET, SOCK_STREAM, getprotobyname('tcp'));

# So we can restart our server quickly
#setsockopt(SERVER, SOL_SOCKET, SO_REUSEADDR, 1);

# Build up my socket address
#my $my_addr = sockaddr_in($server_port, INADDR_ANY);
#bind(SERVER, $my_addr)
#	or die "Couldn't bind to port $server_port : $!\n";
	
# Establish a queue for incoming connections
#listen(SERVER, SOMAXCONN)
#listen(SERVER, 5)
#  or die "Couldn't listen on port $server_port : $!\n";

my $local = IO::Socket::INET->new(
	Proto 	=> 'tcp',
	#LocalAddr=> 'localhost:8080',
	#LocalAddr=> '10.10.10.208:8080',
	LocalAddr=> '0.0.0.0:8080',
	Reuse	=> 1
) or die "$!";

$local->listen();
$local->autoflush(1);

print "SERVER started on port $server_port\n";
print "At your service.  Waiting...\n";
  
# Accept and process connections
my $addr;	# Client handle
#while ($addr = accept(CLIENT, SERVER)) {
while ( $addr = $local->accept() ) {
	# do something with CLIENT
	
	print "Connected from: ", $addr->peerhost();
        #print CLIENT "Hello from server!\n";
	print " Port: ", $addr->peerport(), "\n";

	#my $result;
	my @result;
	my $i = 0;
	while (<$addr>) {	# Read all messages from client
	  last if m/^end/gi;	# Exit loop if 'end' message
	  print "Received: $_";
	  #print $addr $_;	# Send received message back to client
	  print $addr "starting UDP script...";
	  #$result += $_;
	  $result[$i] = $_;
	  #if ($result[$i] eq "start") {
	  if ($result[$i] =~ /^start.*/) {
	    print "starting UDP script...\n";
	    #my $send = "starting UDP script...";
	    print $addr = "starting UDP script...";
            ## Start the flooder
            #system("./udpinject.sh");
            my $status = system("./udpinject.sh");
            
            #unless (defined (my $pid = fork)) {
            #  die "cannot fork: $!";
            #} 
            #else {
            ## unless (my $pid) {
            #   my $status = exec ("udpinject.sh");
            #   waitpid($pid, 0);
            #   last;
            #}

	  } elsif ($result[$i] =~ /^\d+$/) {
	    print "running for $result[$i] seconds\n";
            my $send = "running for $result[$i] seconds";
	  } else {
	    print "nice try!\n";
            my $send = "nice try!";
	  }
	  $i++;
	}
	chomp;
	if (m/^end/gi) {	# Needed to prevent client termination
	  my $send = "end of socket message";
	  print $addr "$send\n";
	  #print "Result: $send\n";
	}
	print "Closed connection\n";
	close $addr;

	print "At your service.  Waiting...\n";

	#close(SERVER);
	#SERVER->autoflush(1);

# Wait for next request
}

close(SERVER);
