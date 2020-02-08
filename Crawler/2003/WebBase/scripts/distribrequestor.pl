#!/usr/bin/perl -w
# client to access distribdaemon

# args: (must be in this order)
#     host
#     port
#     numpages
#     starting url (optional)
#     ending url (optional)
#     offset  (op-tional)
#   Gary Wesley <gary@db.stanford.edu> 5/01

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
use IO::Socket;
use Env;
my $pages;
my $starturl;
my $endurl;
my $host = "wb1";
my $port = 7008;
my $offset = 0;
#printf  @ARGV . "\n";
if(@ARGV < 1) {
 printf("Enter host: ");
 chop($_ = <STDIN>);
 $host = $_;
  printf "host $host\n";
}
else {
  $host = $ARGV[0];
}
#die "Use ehdistribrequestor.pl for 2001 crawl" if( $host eq "eh" );
my $fullhost = "$host.stanford.edu";


if(@ARGV < 2) {
  printf("Enter port: ");
  chop($_ = <STDIN>);
  $port = $_;
  die "Non-numeric, try again..." unless /\d/;
  printf "port $port\n";
}
else { 
  $port= $ARGV[1];
}

if(@ARGV < 3) {
  printf("Enter number of pages required: ");
  chop($_ = <STDIN>);
  die "Non-numeric, try again..." unless /\d/;
  $pages = $_;
  printf "$pages pages requested\n";
}
else {
  $pages = $ARGV[2];
}

if(@ARGV > 3) {
  $starturl = $ARGV[3];
  chomp  $starturl;
  if(@ARGV > 4) {
    $endurl = $ARGV[4];
    
    if(@ARGV > 5) {
      $offset = $ARGV[5];
}}}
$EOL = "\015\012";


$remote = IO::Socket::INET->new( Proto     => "tcp",
				 PeerAddr  => $fullhost,
				 PeerPort  => $port ,
			       );
unless ($remote) { die "cannot connect to  daemon on $fullhost" }
$remote->autoflush(1);


#actual call
my $remoteline =  "new";
if(defined($offset   ) ){ $remoteline .= ",$offset"; }
                   else { $remoteline .= ",0"; }
if(defined($starturl ) ){ $remoteline .= ",$starturl"; }
if(defined($endurl   ) ){ $remoteline .= ",$endurl"; }
$remoteline .= ",$EOL";
#print "remoteline= $remoteline";
print $remote $remoteline;
my $line = "";
read( $remote, $line, 200 );     # read reply (1st 200 chars) into $line
close $remote;

(my $ip,my $remotePort) = split(/ /, $line); # parse line into 2 tokens

print "distrib daemon returned $line \n(use as  ../bin/RunHandlers ../inputs/webbase.conf \"net://" . $ip . ":" . $remotePort . "/?numPages=" . $pages . "\"  )" ;

print "\n";
close $remote;

exit;
