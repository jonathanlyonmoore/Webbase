#!/usr/bin/perl -w
#
# client to access distributor page feeder
#
#  optional args:   num-pages host port firstSite lastSite
#
#    to only get one site: specify firstSite & lastSite (equal)
#
# Description:
#  issue the process command after getting ip address & port from 
#      distribdaemon
#
#   Gary Wesley <gary@db.stanford.edu> 12/02

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
#use diagnostics;

require 5.005; # least we have tested on
use IO::Socket;

printf "Starting $0 using Perl  %vd\n", $^V ;

my $host;

my $EOL = "\015\012";
my $offset;
my $pages  = 0;
my $pgpos = 0;
my $hostpos = 1;
my $portpos = 2;
my $firstpos = 3;
my $lastpos = 4;
my $offsetpos = 5;
my $starturl;
my $endurl;

# Find the path where the WebBase code tree begins.
use FindBin;
$_ = $FindBin::Bin;
s#/([^/]+)$#/# ; # happens to end in a slash
my $WEBBASE = $_;
my $port;

my $logFile  = "$WEBBASE/scripts/gplog"; 
if(@ARGV < 1) { 
  printf "usage: getpages.pl num-pages host port firstSite lastSite\n";
  exit 1;
}
if(@ARGV < ($pgpos + 1 )) {
  printf("Enter number of pages required: ");
  chop($_ = <STDIN>);
  $pages = $_;
  printf "$pages pages requested\n";
}
else {
  $pages = $ARGV[ $pgpos ];
}
die "pages must be numeric\n" unless $pages =~ /^\d+$/;

if(@ARGV < ( $hostpos + 1 )) {
  $host = "WB1";
}
else { 
  $host = $ARGV[ $hostpos ];
}
my $fullhost;
die "Use ehgetpages.pl for 2001 crawl" if( $host eq "eh" );

if(@ARGV < $portpos + 1) {
  $port = `cat $WEBBASE/scripts/portnum`; 
  if( $port < 1000 ){ $port = 7008; } # could not find file
}
else { 
  $port= $ARGV[ $portpos ];
}
die "port must be numeric\n" unless $port =~ /^\d+$/;

if(@ARGV > $firstpos ) {
  $starturl = $ARGV[ $firstpos];
  chomp $starturl; print "starturl set to $starturl|\n";
}
if(@ARGV > $lastpos) {
  $endurl = $ARGV[$lastpos];print "endurl set to $endurl\n";
}
if(@ARGV > $offsetpos) {
  $offset = $ARGV[$offsetpos];print "offset set to $offset\n";
}
#print $offset .  $pages . " from " . $host . ":"  . $port . " . " . $starturl . " . " . $endurl . "\n";

#build command
my $cmd = "perl distribrequestor.pl $host $port $pages";
if(defined($starturl ) ){$cmd .= " ";$cmd .= $starturl;}
if(defined($endurl   ) ){$cmd .= " ";$cmd .= $endurl; }
if(defined($offset   ) ){$cmd .= " ";$cmd .= $offset;}
#printf "command is |$cmd|\n";
chomp $cmd;
my $ans = `$cmd`;

#parse answer
#    printf "answered by:$ans\n";
#distrib  daemon  returned 171.64.75.151 7136
$ans =~ /(.*) (\d\d\d.\d\d.\d\d.\d\d\d) (\d\d\d\d)/;
my $ip;
die unless $ip = $2;
my $remotePort = $3;
#printf "$ip|$remotePort|\n";

die "Could not get a distributor" unless $port =~ /^\d+$/;

# Let them go ahead and run if they want
my $commandLine = "$WEBBASE/bin/RunHandlers $WEBBASE/inputs/webbase.conf \"net://$ip:$remotePort/?numPages=$pages";
if( defined ($offset )){ $commandLine .= "\&offset=$offset";}
$commandLine .= "\"";# > $logFile";
sleep 8; # host is often very busy on ports

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
