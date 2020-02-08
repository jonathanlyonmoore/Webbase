#!/usr/bin/perl
#
# runhandlers | ./extract-hosts.pl > hosts.txt
#
# Enable CATLINKS in the conf file for runhandlers
#
# will not work with 5.8.0

#use warnings;
#use strict;
#use diagnostics;

use URI;
$COMPAT_VER_3 = 1;
#printf "Try COMPAT_VER_3\n";
my $quit = 0;

$SIG{QUIT} = $SIG{INT} = sub { $quit = 1; print STDERR "Quit requested...\n"};

my $cnt = 0;

my %hostnames;

while(<>) {
  last if $quit;
  chomp;
  if(/^http/i) {
    eval {
      my $uri = URI->new($_)->canonical;#printf $uri ."|" .  $uri->scheme . "\n";
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
