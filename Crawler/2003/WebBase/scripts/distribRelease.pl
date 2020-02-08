#!/usr/bin/perl -w
# client to release a distributor
# 
#   Gary Wesley <gary@db.stanford.edu> 4/02

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
require 5.005; # least we have tested on
use IO::Socket;
use Env;
my $host;
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

my $daemonPORT;
if(@ARGV < 2) {
  printf("Enter port to talk to: ");
  chop($_ = <STDIN>);
  $daemonPORT = $_;
  die "Non-numeric, try again..." unless /\d/;
  printf "ask on port $daemonPORT\n";
}
else { 
  $daemonPORT= $ARGV[1];
}

my $relPort;
if(@ARGV < 3) {
  printf("Enter port to release: ");
  chop($_ = <STDIN>);
  $relPort = $_;
  die "Non-numeric, try again..." unless /\d/;
  printf "release port $relPort\n";
}
else { 
  $relPort= $ARGV[2];
}
my $EOL = "\015\012";

my $remote = IO::Socket::INET->new( Proto     => "tcp",
				 PeerAddr  => $fullhost,
				 PeerPort  => $daemonPORT ,
			       );
unless ($remote) { die "cannot connect to  daemon on $host" }
$remote->autoflush(1);
chomp $relPort;

#actual call
print $remote "done,$relPort$EOL";
my $line = "";
read( $remote, $line, 100 );     # read reply (1st 100 chars) into $line
close $remote; 

print "distrib daemon returned: $line";
print "\n";
close $remote; 

exit;
