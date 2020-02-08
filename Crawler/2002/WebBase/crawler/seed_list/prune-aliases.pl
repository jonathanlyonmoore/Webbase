#!/usr/bin/perl
#
# Prunes out aliases whose robots.txt files don't match
#

use warnings;
use strict;

#use LWP::Simple;
use LWP::UserAgent;
use Digest::MD5 qw(md5_hex);
use Socket;

# create agent
my $ua = new LWP::UserAgent;

$ua->timeout(15);
$ua->max_size(500_000);

$| = 1;

$/ = "";

while(<>) {
  chomp;
  my @line = split(/\n/, $_);
  my $head = (split(' ', (shift @line)))[1];
  my @aliases = split(' ', shift @line); # bf
  my @ipaddrs = split(' ', shift @line); #bf
  my $pagerank = (split(' ', (shift @line)))[1];
  shift @aliases; shift @ipaddrs;

  my %aliases;
  foreach my $alias (@aliases) { $aliases{$alias} = 1; }
  delete $aliases{$head};
  @aliases = sort keys %aliases;

  my @true_aliases = ();
  if(@aliases > 0) {
    my $robots_file;
    {
      my $request = HTTP::Request->new('GET', "http://$head/robots.txt");
      my $response = $ua->request($request);
      if($response->is_success) {
	$robots_file = $response->content();
      }
    }
    my $head_robots_md5 = "0";
    $head_robots_md5 = md5_hex($robots_file) if defined($robots_file);
#    print STDERR "ROBOTS FILE\n$robots_file\nEND\n\n";
    foreach my $alias (@aliases) {
      # check robots file
      my $alias_robots_file;
      {
	my $request = HTTP::Request->new('GET', "http://$alias/robots.txt");
	my $response = $ua->request($request);
	if($response->is_success) {
	  $alias_robots_file = $response->content();
	}
      }
      my $alias_robots_md5 = "0";
      $alias_robots_md5 = md5_hex($alias_robots_file) if defined($alias_robots_file);
#      print STDERR "ALIAS ROBOTS FILE\n$alias_robots_file\nEND\n";
      if($head_robots_md5 eq $alias_robots_md5) {
	push @true_aliases, $alias;
      }
      else {
	print STDERR "dropping alias for $head: $alias\n";
      }
    }
  }

  print "head: $head\n";
  print "aliases: " . join(" ", @true_aliases) . "\n";
  print "addrs: " . join(" ", @ipaddrs) . "\n";
  print "pagerank: " . $pagerank . "\n\n";
}

