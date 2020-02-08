#!/usr/bin/perl
#
# Loads the crawl entries into the database
#

# 
=comment
 SQL for creating the relevant tables:

create table Hosts(
hid BIGINT PRIMARY KEY, 
hostname varchar(1024) UNIQUE NOT NULL, 
priority REAL NOT NULL, 
minpages INTEGER, maxpages INTEGER NOT NULL, 
mindepth INTEGER, maxdepth INTEGER, 
imagesp BOOLEAN, pdfsp BOOLEAN, audiop BOOLEAN);

create table Aliases(hid BIGINT NOT NULL, alias varchar(1024) NOT NULL);

create table IPaddrs(hid BIGINT NOT NULL, ipaddr INET NOT NULL);

  Create indexes for the foreign keys:

create index aliases_hid_skey on Aliases(hid);
create index ipaddrs_hid_skey on IPAddrs(hid);
=cut

use warnings;
use strict;

use DBI;

$/ = "";

my $dbh = DBI->connect("dbi:Pg:dbname=webbase;host=oh", "webbase", "wbwb");

my $hosts_sth = $dbh->prepare(q{
                 INSERT INTO Hosts(hid, hostname, priority, maxpages, maxdepth)
			VALUES (?, ?, ?, 20000, 10)
		      });
my $ipaddrs_sth = $dbh->prepare(q{
                      INSERT INTO IPaddrs(hid, ipaddr)
			VALUES (?, ?)});
my $aliases_sth = $dbh->prepare(q{
                      INSERT INTO Aliases(hid, alias)
			VALUES (?, ?)});

my %aliasmap = ();

my $hid = 0;
while(<>) {
  chomp;
  my @line = split(/\n/, $_);
  my $head = (split(' ', (shift @line)))[1];
  my @aliases = split(' ', shift @line); # bf
  my @ipaddrs = split(' ', shift @line); #bf
  shift @aliases; shift @ipaddrs;
  die "Not enough ips!" unless @ipaddrs;
  my $pagerank = (split(' ', (shift @line)))[1];

  $hosts_sth->execute($hid, $head, $pagerank);

  foreach my $ipaddr (@ipaddrs) {
    $ipaddrs_sth->execute($hid, $ipaddr);
  }

  foreach my $alias (@aliases) {
    print STDERR "Dup: $head -> $alias\n" if(exists $aliasmap{$alias});
    $aliasmap{$alias}++;
    $aliases_sth->execute($hid, $alias);
  }

  $hid++;
#  last if $hid > 100;
}

$dbh->disconnect;
