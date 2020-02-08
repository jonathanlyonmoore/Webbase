#!/usr/bin/perl -w
# client to access distribdaemon
# writes to a conf file for feeder to use
#   Gary Wesley <gary@db.stanford.edu> 3/01
use IO::Socket;
print "Deprecated: use distribrequestor.pl or getpages.pl\n";
unless (@ARGV == 1) { 
  print "usage: $0 starting-offset (0=beginning)\n" ; 
  exit(1); }
$EOL = "\015\012";

$host = "eh.stanford.edu";

$PORT= 7002;
 
$remote = IO::Socket::INET->new( Proto     => "tcp",
				 PeerAddr  => $host,
				 PeerPort  => $PORT ,
			       );
unless ($remote) { die "cannot connect to  daemon on $host" }
$remote->autoflush(1);


#actual call
print $remote "new,$ARGV[0]$EOL";

print "distrib daemon returned (use as ./RunHandler net://<IP>:<port>) : ";
while ( <$remote> ) { 
  print;
}
print "\n";
close $remote; 

exit;
