#!/usr/bin/perl -w
# check the distribdaemon
use IO::Socket;

my $host = "eh.stanford.edu";
my $EOL = "\015\012";
#$BLANK = $EOL x 2;
my $PORT = `cat portnum`;
  
$remote = IO::Socket::INET->new( Proto     => "tcp",
				 PeerAddr  => $host,
				 PeerPort  => $PORT ,
			       );
unless ($remote) { die "cannot connect to  distrib daemon on $host >>>>>>>>>>>>>>>>>>>>>>>>>>>>\n" }
$remote->autoflush(1);

#actual call
print $remote "ping,0$EOL";
close $remote;
print "checking distributors/daemon ---------------------   ";
print "distribdaemon is up  \n";

#show non-stanford distributor requests since 10/9/1
my $cmd = "grep \"new |\" /dfs/sole/6/gary/dli2/src/WebBase/scripts/ddaemon.log | grep -v anford | tail";
#my $cmd = "grep \"new |\" /dfs/sole/6/gary/dli2/src/WebBase/scripts/.nfs000302d50000001c | grep -v anford | tail -1";
system( $cmd );
print "----------------------------------------------------------\n";
exit;  
