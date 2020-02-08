#!/usr/bin/perl
#
# runhandlers | ./extract-hosts.pl > hosts.txt
#
# Enable CATLINKS in the conf file for runhandlers
#


use warnings;
use strict;
use diagnostics;

use URI;

my $quit = 0;

$SIG{QUIT} = $SIG{INT} = sub { $quit = 1; print STDERR "Quit requested...\n"};

my $cnt = 0;

my %hostnames;

while(<>) {
  last if $quit;
  chomp;
  if(/^http/i) {
    eval {
      my $uri = URI->new($_)->canonical;
      $hostnames{$uri->host}++ if ($uri and $uri->scheme eq "http");
    };
    if($@) {
      print STDERR "Error: $@\n";
    }
  }
  output() if(++$cnt % 1000000 == 0);
}

output();

sub output {
  print STDERR "cnt: $cnt\n";
  foreach my $hostname (keys %hostnames) {
    print "$hostname\t$hostnames{$hostname}\n";
  }
  %hostnames = ();
}
