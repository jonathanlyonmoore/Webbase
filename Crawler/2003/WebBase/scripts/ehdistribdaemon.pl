#!/usr/bin/perl -w
#
#distribdaemon.pl - service request for  request for a distributor
# client parms: <cmd>,<offset> 
# commands: new,0
#             (offset = -1 = wherever you left off )
#           ping
#           done,[port] I'm done, kill this port
#
# run from scripts directory

# runs on Linux or Solaris

# Gary Wesley <gary@db.stanford.edu> 3/01
#
use strict;
use Socket;
require 5.005; # least we have tested on

# Leave an old one running on old port number when you change ports #########


#############################################################################
my $DISTRIB_PORT     = 9000; # start here and check
my $RESTART_PORTS_AT = 2000;

# Find the path where the WebBase code tree begins.
use FindBin;
$_ = $FindBin::Bin;
s#/([^/]+)$#/# ; # happens to end in a slash
my $WEBBASE = $_;
my $MYPORT= `cat $WEBBASE/scripts/portnum.2001`;

my $INPUT_FILE     = "../inputs/rep2001";
my $LOGFILE        = "$WEBBASE/scripts/ddaemon.log.2001";

# for checking available fs slots
my $fsstat;
# There is no global limit on the number of file descriptors in Solaris.
if ($ENV{OSTYPE} ne 'sunos') { $fsstat = `cat /proc/sys/fs/file-nr`; }

my $ALLOCATED;
my $LEFT;
my $USED;
my $MAX ;
my $SAVESOMEFS = 800;  # save a buffer of file handles

# hash of user ip / port 
my %userLookup = ();

open(LOG, ">> $LOGFILE") || die "Open $LOGFILE: $!";

# look up my environment
use Sys::Hostname;
my $iaddr      = gethostbyname(hostname());
my $MY_IP_ADDR = inet_ntoa(    $iaddr ); print "$MY_IP_ADDR\n";
my $HOST       = gethostbyaddr($iaddr,AF_INET);
$HOST          = hostname();
$ENV{LD_LIBRARY_PATH} = "lib";

printf LOG "==================================================================\n";
printf LOG "Starting $0 using Perl %vd ", $^V; #$PERL_VERSION;
printf LOG " on port $MYPORT to get distributor on $HOST";
printf LOG " using input file $INPUT_FILE at " . localtime() . "\n";

# open a socket
use IO::Socket;
my $sock;
$sock = new IO::Socket::INET (LocalPort => $MYPORT,
			      Proto => 'tcp',
			      Listen => 5,
			      Reuse => 1) ||
  die "Socket error:  $!\n" unless $sock;
printf LOG "$HOST Listening on port $MYPORT...\n";
printf LOG "==================================================================\n";

##############################################################################################
while (1) {  # daemonic!
    if( $DISTRIB_PORT > 32760 ){ 
	$DISTRIB_PORT =  $RESTART_PORTS_AT; 
	printf LOG "!!!!!!!!!!!! R E C Y C L E !!!!!!!!!!!!!!!!!!!!!restart at $RESTART_PORTS_AT";
    }

    # good-enough-for-now check for busy port
    # we check each time in case someone has grabbed it
    my $portUsed = 1;
    while ( $portUsed ){
	my $resp = `netstat -a`;
	#print "$resp";
	if( $resp =~ /tcp.*:$DISTRIB_PORT\b/) { 
	    printf LOG "$DISTRIB_PORT used, incrementing...\n";
	    $DISTRIB_PORT += 1;
	    if( $DISTRIB_PORT > 32700 ){ 
		$DISTRIB_PORT =  $RESTART_PORTS_AT; 
		printf LOG "!!!!!!!!!!!! R E C Y C L E !!!!!!!!!!!!!!!restart at $RESTART_PORTS_AT";
	    }
	}
	else{  $portUsed = 0; }
    }
    
    #accept and echo
    my $input_sock = $sock->accept() || die "socket: $!"; # wait here
    my $ip = $input_sock->peerhost() || die "peerhost: $!";
    (my $name,my $alias,my $addrtype,my $length,my $new_addr) = 
	gethostbyaddr(inet_aton($ip),AF_INET);
    printf LOG localtime() . " $name $ip $alias Port $MYPORT";
    
    #parse input
    my $RAWOFFSET = 0; # in case they don't send one
    (my $COMMAND, $RAWOFFSET )  =  split(',',  <$input_sock>);
    chop          $RAWOFFSET;
    chop( my $OFFSET = $RAWOFFSET );
    
    printf LOG " $COMMAND | ";
    
    if ( $COMMAND eq "new") {  # fire up a distributor and give me his port/ip
	print LOG " Client passed us offset:$OFFSET\n";

	if ($ENV{OSTYPE} ne 'sunos') {
	  # make sure fs table is not filling up
	  ( $ALLOCATED, $USED, $MAX )  =  split(' ',  $fsstat);
	  $LEFT = $MAX - $USED;
	  printf LOG "alloc $ALLOCATED used $USED max $MAX left $LEFT " ;	
	}
	else { #fake it
	  $LEFT = 1;
	  $SAVESOMEFS = 2;
	}
	if ( $LEFT lt $SAVESOMEFS ){
	    #printf "$LEFT file descriptors left";
	    

            # start up distributor
	    my $START_COMMAND = "nohup ../bin/distributor.2001 -p $DISTRIB_PORT -f ../inputs/webbaseServer.conf -u \"rep://localhost/$INPUT_FILE?offset=$OFFSET&compress=1\" &";
	    # my $START_COMMAND = "nohup ../bin/distributor -p $DISTRIB_PORT -u rep://localhost/$INPUT_FILE?offset=$OFFSET&compress=1 &";
	    # my $START_COMMAND = "nohup bin/distributor -p$DISTRIB_PORT -s$OFFSET -z -f$INPUT_FILE -h &";    
	    printf LOG "$START_COMMAND\n";	    
	    my $rc = system( $START_COMMAND );
	    print "start distributor returns: $rc\n";
	    
	    # return info to client
	    printf LOG "return $MY_IP_ADDR $DISTRIB_PORT \n";
	    print $input_sock "$MY_IP_ADDR $DISTRIB_PORT";
	    $userLookup{ $DISTRIB_PORT } = $ip;
	    print  "stored hash: $userLookup{$DISTRIB_PORT} for port $DISTRIB_PORT\n";
	    $DISTRIB_PORT += 1;
	}
	else {
	    # too many open
	    printf LOG "return 0:0 , saving $SAVESOMEFS\n";
	    print $input_sock "0 0";
	}
    }
    elsif( $COMMAND eq "ping") {
	#printf LOG "ping from:  $name $ip $alias \n";
	print $input_sock "0 0";
    }
    elsif ( $COMMAND eq "done") {  
	my $USER;
	my $PID = 0;
	my $REST;
	my $rc;
	my $KILLPORT = $OFFSET;
	printf LOG "Client passed us port to kill:$KILLPORT\n";
	#printf  "$userLookup{$KILLPORT} $ip";
	if($userLookup{$KILLPORT} eq $ip ){
	    
	    my $PS_COMMAND = "ps -eaf | grep distributor | grep $KILLPORT";
	    printf "$PS_COMMAND\n";
	    #gary     30141     1  0 05:30 pts/1    00:00:00 ../bin/distributor -p 9082 -f 
	    my $resp =  `$PS_COMMAND`;
	    
	    #print "ps returns: $resp\n";
	    
	    ($USER, $PID, $REST )  =  split('     ', $resp );
	    #chop $PID;
	    if( $PID > 0 ){
		printf "$PID\n";
		
		# kill kill kill
		my $K_COMMAND = `kill $PID`;
		$rc = system( $K_COMMAND );
		printf LOG "kill $PID for port $KILLPORT returns: $rc\n";
		print $input_sock "OK";
                # keep from trying to kill later
		$userLookup{ $KILLPORT } = 0; 
		
		#$resp = `$PS_COMMAND`;
		#($USER, $PID, $REST )  =  split('     ', $resp );
		#$K2_COMMAND = `kill -9 $PID`;
		#$rc = system( $K2_COMMAND );
		#printf LOG "kill returns: $rc\n";
	    }
	}
	elsif (! $userLookup{$KILLPORT} ) {
	    print $input_sock "Process already ded on port $KILLPORT";
	}
	else {
	    print $input_sock "Cannot kill process on port $KILLPORT, NOT same ip addr as owner";
	    printf LOG ">>>>>>>>>>>>>>>>>> USER ERROR >>>>>>>>>>>>>>>> ";
	    printf LOG "NOT same user as port owner $userLookup{$KILLPORT} vs $ip\n";
	}
    }
    else { print $input_sock "-1 -1";} # unknown
    
    close($input_sock); 
    printf LOG "client sock closed---------------------------------\n";
    
}

exit;
