#!/usr/bin/perl
#
# A simple perl server for providing WebBase data
# Adapted the server from the camel book page 350/351
#
# Author: Taher H. Haveliwala
#
# Usage: simpleserver.pl portno webbase.conf
#

use warnings;
use strict;

BEGIN { 
  $ENV{PATH} = '/usr/bin';
  $ENV{PGUSER} = 'webbase';
  $ENV{PGPASSWORD} = 'wbwb';
}

use lib "/u/taherh/research/latest/WebBase/perllib/";

use Socket;
use Carp;
use FileHandle;
use IO::Socket::INET;
use WB::Properties;

die "Usage: simpleserver.pl portno webbase.conf" if(@ARGV != 2);

sub spawn;  # forward declaration
sub logmsg { print STDERR "$0 $$: @_ at ", scalar localtime, "\n" }

my $port = shift;
$port = $1 if $port =~ /(\d+)/; # untaint port number

(*Server = IO::Socket::INET->new(Listen     => 5,
				 LocalPort  => $port,
				 Proto      => 'tcp'))
  or die "Couldn't open socket";

logmsg "server started on port $port";

my $waitedpid = 0;
my $paddr;

sub REAPER {
    $waitedpid = wait;
    $SIG{CHLD} = \&REAPER;  # loathe sysV
    logmsg "reaped $waitedpid" . ($? ? " with exit $?" : '');
}

$SIG{CHLD} = \&REAPER;

for ( $waitedpid = 0;
      ($paddr = accept(Client,Server)) || $waitedpid;
      $waitedpid = 0, close Client)
{
    next if $waitedpid and not $paddr;
    my($port,$iaddr) = sockaddr_in($paddr);
    my $name = gethostbyaddr($iaddr,AF_INET);

    logmsg "connection from $name [", inet_ntoa($iaddr), "] at port $port";

    spawn \&handler;
}

sub spawn {
    my $coderef = shift;

    unless (@_ == 0 && $coderef && ref($coderef) eq 'CODE') {
	confess "usage: spawn CODEREF";
    }

    my $pid;
    if (!defined($pid = fork)) {
	logmsg "cannot fork: $!";
	return;
    } elsif ($pid) {
	logmsg "begat $pid";
	return; # I'm the parent
    }
    # else I'm the child -- go spawn

    open(STDIN,  "<&Client")   || die "can't dup client to stdin";
    open(STDOUT, ">&Client")   || die "can't dup client to stdout";
    # open(STDERR, ">&STDOUT") || die "can't dup stdout to stderr";
    STDOUT->autoflush();
    exit &$coderef();
}

sub handler {
  my $input;

  while($input = <STDIN>) {
    my($command, $url) = split(' ', $input);

    if($command eq "F") {
      logmsg "Command: F $url";

      print `echo $url | ../bin/normalize-test | ./linklookup.pl forward ../sampleInputs/webbase.conf`;
      print "\nEND_WB_TRANS\n";
    }
    elsif($command eq "B") {
      logmsg "Command: B $url";

      print `echo $url | ../bin/normalize-test | ./linklookup.pl backward ../sampleInputs/webbase.conf`;
      print "\nEND_WB_TRANS\n";
    }
    elsif($command eq "T") {
      logmsg "Command: T $url";

      print WB::Properties::get_title($url);
      print "\nEND_WB_TRANS\n";
    }
    elsif($command eq "P") {
      logmsg "Command: P $url";

      print WB::Properties::get_pagerank($url);
      print "\nEND_WB_TRANS\n";
    }
    elsif($command eq "C") {
      logmsg "Command: C $url";

      my $offset = WB::Properties::get_offset($url);

      logmsg "Offset: $offset";

      if($offset eq "") {
	  logmsg "No info";
	  print "NO_INFO\n";
      }
      else {
	  logmsg "Executing RunHandlers on offset $offset to output page";
	  print `echo $offset | ../bin/RunHandlers ../inputs/webbase.conf "rep://localhost/../inputs/rep2001?compress=0" -`;
      }
      print "\nEND_WB_TRANS\n";
    }
    else {
      logmsg "Command: $command $url";

      print "Unsupported\n";
      print "\nEND_WB_TRANS\n";
    }
  }
  return 0;
}
