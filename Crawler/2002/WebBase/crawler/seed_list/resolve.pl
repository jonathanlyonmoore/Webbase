#!/usr/bin/perl
#
# A preprocessor that takes a list of hostnames, and generates a crawl list
# for the crawler.
#

use warnings;
use strict;

use LWP::Simple;
use Socket;

$| = 1;

while(<>) {
  my @line = split;
  my $pagerank = pop @line;
  my ($name,$aliases,$addrtype,$length,@addrs) = gethostbyname($line[0]);
  print "original: @line\npagerank: $pagerank\nname: $name\naliases: $aliases\naddrtype: $addrtype\nlength: $length\naddrs: " . join(" ", (map {inet_ntoa($_)} @addrs)) . "\n\n" if $name;
}
