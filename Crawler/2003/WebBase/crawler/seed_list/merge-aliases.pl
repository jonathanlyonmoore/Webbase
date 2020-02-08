#!/usr/bin/perl
#
# Takes the dns entries from resolve.pl and tries to find and match up
# aliases.  (www).foo.com is automatically tested as an alias, regardless
# of what dns says.
#

use warnings;
use strict;

$/ = "";

my %records;
my %map;

while(<>) {
  chomp;
  my %entry;
  my @lines = split(/\n/, $_);
  my $head = (split(' ', $lines[0]))[1];

  # its a numeric name
  next unless $head =~ /\w/;

  # a higher pageranked head owns this hostname
  next if(exists $map{$head});

  # make this hostname an alias of a previous entry if the only
  # difference is the www
  my $other = "";
  my @components = split(/\./, $head);
  if($components[0] eq "www") {
    $other = join(".", @components[1..$#components]);
  }
  else {
    $other = "www.$head";
  }

  my @ipaddrs = split(' ', $lines[6]);
  @ipaddrs = sort @ipaddrs[1..$#ipaddrs];

  if(exists $records{$other} 
     and 
     $records{$other}{ipaddrs}[0] eq $ipaddrs[0]
    ) {
    push @{$records{$other}{aliases}}, $head;
    push @{$map{$head}}, $other; 
    next;
  }

  # this isn't an alias, so create a new crawl entry

  $records{$head}{pagerank} = (split(' ', $lines[1]))[1];

  my @aliases = ($head);
#  push @aliases, (split(' ', $lines[2]))[1];
  my @tmp = split(' ', $lines[3]);
  @tmp = @tmp[1..$#tmp];
  push @aliases, @tmp;
  $records{$head}{aliases} = \@aliases;
  
  # store alias->head mapping
  foreach my $alias (@aliases) {
    push @{$map{$alias}}, $head;
  }

  $records{$head}{ipaddrs} = \@ipaddrs;
}

foreach my $head (sort 
		  {$records{$b}{pagerank} <=> $records{$a}{pagerank}} 
		  keys %records) {
  print "head: $head\n";
  print "aliases: " . join(" ", @{$records{$head}{aliases}}) . "\n";
  print "addrs: " . join(" ", @{$records{$head}{ipaddrs}}) . "\n";
  print "pagerank: " . $records{$head}{pagerank} . "\n\n";
}
