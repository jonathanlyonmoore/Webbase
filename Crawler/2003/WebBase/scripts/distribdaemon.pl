#!/usr/bin/perl -w
#
#distribdaemon.pl - service request for  request for a distributor
# client parms: <cmd>,<offset>,<starturl>,<endurl>
# script parms : <input file> <port> <data dir> <logfile>
# commands: new,
#             (offset = -1 = wherever you left off ) 
#             ( URL to begin at )
#             ( URL to end   at )
#           ping
#           done,[port] I'm done, kill this port
# run from scripts directory
# runs on Linux or Solaris
# Gary Wesley <gary@db.stanford.edu> 5/03
#

#  The Stanford WebBase Project <webbase@db.stanford.edu>
#   Copyright (C) 2003 The Board of Trustees of the
#   Leland Stanford Junior University
#   
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License
#   as published by the Free Software Foundation; either version 2
#   of the License, or (at your option) any later version.
#   
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#   #
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
use strict;
use Socket;
require 5.005; # least we have tested on

#############################################################################
my $DISTRIB_PORT     = 7400; # start here and check
my $RESTART_PORTS_AT = 7060; # save first few dozen for daemons
my $LASTPORT         = 8999; # 9xxx not opened up in system yet

# Find the path where the WebBase code tree begins.
use FindBin;
$_ = $FindBin::Bin;
s#/([^/]+)$#/# ; # happens to end in a slash
my $WEBBASE = $_;

my $INPUT_FILE;
if(@ARGV < 1) { 
  printf "usage: distribdaemon.pl <input file> <port> <data dir> <logfile>\n";
  exit 1;
}
else {
  $INPUT_FILE = $ARGV[0];
}

my  $MYPORT;
if(@ARGV < 2) { 
  $MYPORT = `cat portnum.4102003`;
}
else {
   $MYPORT  = $ARGV[1];
}
die "numeric port!" unless  /\d/;

my $DATAROOTDIR;
if(@ARGV < 3) {  # assume running from scripts/
  $DATAROOTDIR = "../../../../../../../../lfs/1/tmp/webbase/crawl_test/2003-06/";
}
else {
  $DATAROOTDIR = $ARGV[2];
}
my $cmd = "stat " . $DATAROOTDIR; # verify data files are there
if ( ! `$cmd` ) { # maybe they left off the hokey prefix!
  $DATAROOTDIR = "../../../../../../../.." . $DATAROOTDIR;
  die printf "file not found" . $DATAROOTDIR . "\n" unless `$cmd`;
}

my $LOGFILE;
if(@ARGV < 4) { 
  $LOGFILE = "$WEBBASE/scripts/ddaemon.log.062003";
}
else {
  $LOGFILE = $ARGV[3];
}

system( 'cp ' . $LOGFILE . " " . $LOGFILE . ".save");
my $ERRFILE        = "$WEBBASE/scripts/ddaemon.err.2003";
#system( 'cp ' . $ERRFILE . ' errfile.save.4102003');

# for checking available fs slots
my $fsstat;
my $ALLOCATED;
my $LEFT;
my $USED;
my $MAX ;
my $SAVESOMEFS = 800;  # save a buffer of file handles
my $START_COMMAND;
my $HOST;

# hash of user ip / port 
my %userLookup = ();

open(LOG, ">> $LOGFILE") || die "Open $LOGFILE: $!";
#open(ERR, ">> $ERRFILE") || die "Open $ERRFILE: $!";

# look up my environment
use Sys::Hostname;
#my $iaddr      = getaddrinfo(hostname());
my $iaddr      = gethostbyname(hostname());
my $MY_IP_ADDR = inet_ntoa(    $iaddr ); print "$MY_IP_ADDR  ";
#$HOST       = gethostbyaddr($iaddr,AF_INET);print "  $HOST " ;
$HOST          = hostname();

$ENV{LD_LIBRARY_PATH} = "lib";

printf LOG "==================================================================\n";
printf LOG "Starting $0 using Perl %vd ", $^V; #$PERL_VERSION;
printf LOG " on port " . $MYPORT . " to get distributor on " . $HOST;
printf LOG " using input file $INPUT_FILE at " . localtime() . "\n";

# open a socket

use IO::Socket;
my $sock;
#printf "Get socket\n";
#$sock = new IO::Socket::INET (LocalAddr => "$MY_IP_ADDR:$MYPORT",
$sock = new IO::Socket::INET (LocalPort => $MYPORT,
			      Proto => 'tcp',
			      Listen => 5,
			      Reuse => 1) ||
  die "Socket error:  $!\n" unless $sock;
#printf "Got socket\n";
printf  "$HOST Listening on port $MYPORT...\n";
printf LOG "$HOST Listening on port $MYPORT...\n";
printf LOG "==================================================================\n";

##############################################################################################
while (1) {  # daemonic!
  if( $DISTRIB_PORT > $LASTPORT ){         # fixme : get more ports opened
    $DISTRIB_PORT =  $RESTART_PORTS_AT; 
    printf LOG "!!!!!!!!!!!! R E C Y C L E !!!!!!!!!!!!!!!!!!!!!restart at $RESTART_PORTS_AT";
  }
  
  
  #accept and echo
  my $input_sock = $sock->accept() || die "socket: $!"; # wait here
  my $ip = $input_sock->peerhost() || die "peerhost: $!";
  (my $name,my $alias,my $addrtype,my $length,my $new_addr) = 
    gethostbyaddr(inet_aton($ip),AF_INET);
  printf LOG localtime() . " $name $ip $alias Port $MYPORT";
  #printf localtime() . " $name $ip $alias Port $MYPORT";
  

  # good-enough-for-now check for busy port
  # we check each time in case someone has grabbed it
  my $portUsed = 1;
  while ( $portUsed ){
    # tcp        0      0 *:7409                  *:*                     LISTEN
    my $resp = `netstat -a`;
    #print "$resp";
    if( $resp =~ /$DISTRIB_PORT\b/) { 
      printf LOG "$DISTRIB_PORT used, incrementing...\n";
      $DISTRIB_PORT += 1;
      if( $DISTRIB_PORT > $LASTPORT ){ 
	$DISTRIB_PORT =  $RESTART_PORTS_AT; 
	printf LOG "!!!!!!!!!!!! R E C Y C L E !!!!!!!!!!!!!!!restart at $RESTART_PORTS_AT";
      }
    }
    else{  $portUsed = 0; }
  }
  
  #parse input
  my $RAWOFFSET = 0; # in case they don't send one
  my $STARTURL;
  my $ENDURL;
  my $OFFSET;
  my $COMMAND;
  my $line;
  printf chomp( $line = <$input_sock>);
  ( $COMMAND, $OFFSET, $STARTURL, $ENDURL )  =  split(',', $line );
  if( defined        $OFFSET ){
    chomp             $OFFSET;
    #chop(   $OFFSET = $OFFSET );
  }
  if( defined       $STARTURL ){
    chomp     $STARTURL ; 
    #chop( $STARTURL = $STARTURL );
  }
  if( defined        $ENDURL ){
    chomp            $ENDURL;
    #chop( $ENDURL = $ENDURL );
  }
  
  
  
  printf localtime() . "ddaemon " . $COMMAND;
  if( defined ( $OFFSET   ) ){ printf "|" . $OFFSET ;}
  if( defined ( $STARTURL ) ){ printf "|" . $STARTURL ;}
  if( defined ( $ENDURL   ) ){ printf "|" . $ENDURL;}
  printf  "|\n";
  printf LOG " $COMMAND | ";
  
  if ( $COMMAND eq "new") {  # fire up a distributor and give me his port/ip
    print LOG " Client passed us offset:$OFFSET\n";
    
    if ($ENV{OSTYPE} ne 'sunos') {
      # make sure fs table is not filling up
      $fsstat = `cat /proc/sys/fs/file-nr`;
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
      
      $START_COMMAND = "nohup ../bin/distributor -p $DISTRIB_PORT -f ../inputs/webbaseServer.conf -u \"rep://localhost//$DATAROOTDIR?sites=$INPUT_FILE";
      # start up distributor
      if( defined ($STARTURL) ){
	if($STARTURL =~ /(\w+)/){ # why do I need this?
	  $START_COMMAND .= "&firstSite=$STARTURL";
	}
	if( defined ($ENDURL )){
	  $START_COMMAND .= "&lastSite=$ENDURL";
	}
				       }
      
      $START_COMMAND .= "&compress=1";
      if( defined ($OFFSET) ){$START_COMMAND .=  "&offset=$OFFSET";}
      $START_COMMAND .= "\" &";#> $LOGFILE) &> $ERRFILE &";
      # my $START_COMMAND = "nohup ../bin/distributor -p $DISTRIB_PORT -u rep://localhost/$INPUT_FILE?offset=$OFFSET&compress=1 &";
      # my $START_COMMAND = "nohup bin/distributor -p$DISTRIB_PORT -s$OFFSET -z -f$INPUT_FILE -h &";    
      printf LOG "$START_COMMAND\n";
      printf  "$START_COMMAND\n";
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
    chomp $KILLPORT; # does not work
    chop $KILLPORT;
    printf LOG "Client passed us port to kill:$KILLPORT from $ip\n";
    if($userLookup{$KILLPORT}){ 
      printf  "$userLookup{$KILLPORT} $ip";
      if($userLookup{$KILLPORT} eq $ip ){
	
	my $PS_COMMAND = "ps -ealf | grep distributor | grep $KILLPORT";
	printf "$PS_COMMAND\n";
	#gary     30141     1  0 05:30 pts/1    00:00:00 ../bin/distributor -p 9082 -f 
	#000 S gary     16305     
	my $resp =  `$PS_COMMAND`;
	
	#print "ps returns: $resp\n";
	
	($USER, undef, undef, $PID, $REST )  =  split(' ', $resp );
	
	if( $PID > 0 ){
	  
	  # kill kill kill
	  my $K_COMMAND = `kill $PID`;
	  $rc = system( $K_COMMAND );
	  printf LOG "kill $PID for port $KILLPORT returns: $rc\n";
	  print $input_sock "OK";
	  # keep from trying to kill later
	  $userLookup{ $KILLPORT } = 0;
	}
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
