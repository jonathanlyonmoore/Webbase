#!/usr/bin/perl -w
#
# client to access distributor page feeder
#
#  optional args:   num-pages host port
#
# issue the process command after getting ip address & port from 
#    distribdaemon
#
#   Gary Wesley <gary@db.stanford.edu> 12/02
#
use strict;
#use diagnostics;

require 5.005; # least we have tested on
use IO::Socket;

printf "Starting $0 using Perl  %vd\n", $^V ;

my $host;

my $EOL = "\015\012";
my $offset = 0;
my $pages  = 0;
my $pgpos = 0;
my $hostpos = 1;
my $portpos = 2;


# Find the path where the WebBase code tree begins.
use FindBin;
$_ = $FindBin::Bin;
s#/([^/]+)$#/# ; # happens to end in a slash
my $WEBBASE = $_;
my $port;

my $logFile  = "$WEBBASE/scripts/gplog"; 
# we used to have offset as first arg!
if(@ARGV > 0){
	if ( $ARGV[0] == 0 ){
	  $pgpos++;
	  $hostpos++;
	  $portpos++;
	}
}	
if(@ARGV < ($pgpos + 1 )) {
  printf("Enter number of pages required: ");
  chop($_ = <STDIN>);
  die "Non-numeric, try again..." unless /\d/;
  $pages = $_;
  printf "$pages pages requested\n";
}
else {
  $pages = $ARGV[ $pgpos ];
}

if(@ARGV < ( $hostpos + 1 )) {
  $host = "eh";
}
else { 
  $host = $ARGV[ $hostpos ];
}
my $fullhost = "$host.stanford.edu";

if(@ARGV < $portpos + 1) {
  $port = `cat $WEBBASE/scripts/portnum`; 
  if( $port < 1000 ){ $port = 7003; } # could not find file
}
else { 
  $port= $ARGV[ $portpos ];
}
print $offset .  $pages . " from " . $fullhost . ":"  . $port . "\n";
my $remote = IO::Socket::INET->new( Proto     => "tcp",
				    PeerAddr  => $fullhost,
				    PeerPort  => $port ,
			       );
unless ($remote) { die "cannot connect to daemon on $fullhost on $port $!" }
$remote->autoflush(1);

#call to daemon
print "Requesting distributor at " . localtime() .  "... ";
print $remote "new,$offset$EOL"; # request new feeder, offset of $offset
my $line = "";
read( $remote, $line, 100 );     # read reply (1st 100 chars) into $line
close $remote; 

(my $ip,my $remotePort) = split(/ /, $line); # parse line into 2 tokens
print "\n" . localtime() .  " Got " ,  $ip , ":" , $remotePort , " \n";


# Let them go ahead and run if they want
my $commandLine = "$WEBBASE/bin/RunHandlers $WEBBASE/inputs/webbase.conf \"net://$ip:$remotePort/?numPages=$pages\&offset=$offset\"";# > $logFile";
printf("Do you want to run $commandLine now?(Y/N): ");
chop($_ = <STDIN>);
die "You must answer Y or N, try again..." unless /\w/;
my  $runit = $_;

if(/Y|YES/i){  
  print "Connecting on " , $ip , ":" , $remotePort , "\n";
  
  # make sure . command will work on chub
  # older Linux/Perl requires . command somehow tcsh?
  chdir  "$WEBBASE/handlers" || die "chdir: $!";  

  # start processing pages
  
  print "Issuing command: $commandLine at " . localtime() . "\n";
  system("$commandLine");
  
  my $exit_value = $? >> 8;
  if($exit_value > 0){ print "Exited with a bad: $exit_value\n"; }
  print "Done at " . localtime() . "\n";
}

exit 0;
