#!/usr/bin/perl
#
# Loads the crawl entries into the database
#

# 
=comment
-- SQL for creating the relevant tables:

create table Hosts(
hid BIGINT PRIMARY KEY, 
hostname varchar(1024) UNIQUE NOT NULL, 
priority REAL NOT NULL, 
minpages INTEGER NOT NULL, maxpages INTEGER NOT NULL, 
mindepth INTEGER NOT NULL, maxdepth INTEGER NOT NULL, 
imagesp BOOLEAN NOT NULL, pdfsp BOOLEAN NOT NULL, audiop BOOLEAN NOT NULL,
minpause INTEGER NOT NULL, rooturls TEXT);

create table Aliases(hid BIGINT NOT NULL, alias varchar(1024) NOT NULL);

create table IPaddrs(hid BIGINT NOT NULL, ipaddr INET NOT NULL);

--  Create indexes for the foreign keys:

create index aliases_hid_skey on Aliases(hid);
create index ipaddrs_hid_skey on IPAddrs(hid);
=cut

use warnings;
use strict;

use DBI;

$/ = "";

#my $dbh = DBI->connect("dbi:Pg:dbname=webbase;host=wb1", "webbase", "wbwb");

my $dbh = DBI->connect("dbi:Pg:dbname=webbase-10-2003", "webbase");

my $hosts_sth = $dbh->prepare(q{
                 INSERT INTO Hosts(hid, hostname, priority, maxpages, maxdepth,
				minpages, mindepth, imagesp, pdfsp, audiop,
				minpause)
			VALUES (?, ?, ?, 2000, 10,
				1, 1, 'F', 'F', 'F',
				10000)
		      });
my $ipaddrs_sth = $dbh->prepare(q{
                      INSERT INTO IPaddrs(hid, ipaddr)
			VALUES (?, ?)});
my $aliases_sth = $dbh->prepare(q{
                      INSERT INTO Aliases(hid, alias)
			VALUES (?, ?)});

my %aliasmap = ();
my %headmap = ();

my $hid = 0;
while(<>) {
  chomp;
  my @line = split(/\n/, $_);
  my $head = (split(' ', (shift @line)))[1];
  my $pagerank = (split(' ', (shift @line)))[1];
#  $pagerank /= 100_000;
  my $canname = (split(' ', (shift @line)))[1];
  my @aliases = split(' ', shift @line); # bf
  my $addrtype = (split(' ', (shift @line)))[1];
  my $length = (split(' ', (shift @line)))[1];
  my @ipaddrs = split(' ', shift @line); #bf
  shift @aliases; shift @ipaddrs;
  die "Not enough ips!" unless @ipaddrs;

#  print STDERR "host: $head\npagerank: $pagerank\nipaddrs: @ipaddrs\n\n";

  if($headmap{$head}++ > 0) {
      print STDERR "Dup: $head\n";
      next;
  }
  $hosts_sth->execute($hid, $head, $pagerank);

  foreach my $ipaddr (@ipaddrs) {
    $ipaddrs_sth->execute($hid, $ipaddr);
  }

#  foreach my $alias (@aliases) {
#    print STDERR "Dup: $head -> $alias\n" if(exists $aliasmap{$alias});
#    $aliasmap{$alias}++;
#    $aliases_sth->execute($hid, $alias);
#  }

  $hid++;
#  last if $hid > 100;
}

$dbh->disconnect;
