#!/usr/bin/perl
#
# A preprocessor that takes a list of hostnames, and generates a crawl list
# for the crawler.
#

use warnings;
use strict;

use Socket;

$| = 1;
my $cntr = 0;
while(<>) {
#  print STDERR "$_";
  my @line = split;
  my $pagerank = pop @line;
  my ($name,$aliases,$addrtype,$length,@addrs) = gethostbyname($line[0]);
  print "original: @line\npagerank: $pagerank\nname: $name\naliases: $aliases\naddrtype: $addrtype\nlength: $length\naddrs: " . join(" ", (map {inet_ntoa($_)} @addrs)) . "\n\n" if $name;

  print STDERR "." if(++$cntr % 1000 == 0);
}
