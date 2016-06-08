#!/usr/bin/perl -w

# Filename: client.pl

use strict;
use Socket;

# Initialize host and port
my $host = shift || 'localhost';
my $port = shift || 8080;
my $server = "localhost";  # Host IP running the server

# Create the socket, connect to the port
socket(SOCKET, PF_INET, SOCK_STREAM, (getprotobyname('tcp'))[2])
	or die "Can't create a socket $!\n";
my $remote = connect(SOCKET, pack_sockaddr_in($port, inet_aton($server)))
	or die "Can't connect to port $port!\n";

my $line;
#$remote->autoflush(1);	# Send immediately
SERVER->autoflush(1);	# Send immediately
while ($line = <SOCKET>) {
	print "$line\n";
	last if m/^end/gi;
	my $line = <$remote>;
	if ($line ne $_) {
	  print "Error in sending output\n";
	  exit;
	}
}
my $res = <$remote>;
$res =~ m/result=(\d*)/gi;	# Get numeric result from message
print "Result: $1\n";
print "End of client\n";
close $remote;

close SOCKET or die "close: $!";

