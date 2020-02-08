#!/usr/bin/perl
#
# CGI script for handling keyword search queries
#
# Author: Taher H. Haveliwala (taherh@cs.stanford.edu)
#

use warnings;
use strict;

use lib "/u/taherh/research/latest/WebBase/perllib/";

use Socket;
use IO::Socket::INET;
use FileHandle;
use CGI;
use URI::Escape;

use WB::Properties;
use DMOZ qw(@topics);

STDOUT->autoflush();

my $q = new CGI;

# the query
my $query_string = $q->param('query');

my $autodetect = 0;
my $qhistory = 0;

if (not $query_string) {
  print $q->header;
  print '<html>
          <head><title>No query</title>
          <body><p>You did not enter a query!</p></body>
         </html>
        ';
  exit(0);
}

# clean up query
$query_string =~ s/\"/ /g;
# force conjunctive query
$query_string = join(" ", map {/^\+/?"$_":"+$_"} split(' ', $query_string));

# connect to text indexing client
*TISOCK = IO::Socket::INET->new("oh.stanford.edu:7444");

my $cmd = "100";

if($autodetect) { $cmd .= " y"; }
else { $cmd .= " n"; }

foreach my $i (0..15) {
  $cmd .= " 0";
}

my @prev_words = ();
$cmd .= " " . scalar(@prev_words) . " @prev_words";

$cmd .= " $query_string\n";

print TISOCK $cmd;

# get back the simvec used
my $simvec_str = <TISOCK>;
my ($str, @simvec) = split(' ', $simvec_str);
if($str ne "sim" or $#simvec != 16) {
  print "Error: simvec_str = $simvec_str\n";
  exit(0);
}

print "<HTML>\n";
print "<HEAD><TITLE>WB Demo - Results for $query_string</TITLE></HEAD>\n";
print "<BODY>\n";
print "<h1 align=center>Index Demo: $query_string</h1>\n";

my @qwords = split(' ', $query_string);
map {$_ =~ s/\+//} @qwords;

my @non_title_matches = ();

print "<ul>";
while(<TISOCK>) {
  my($url, $rank) = split;
  my $title = WB::Properties::get_title("$url");
  my $found = 0;
  my $word;
  foreach $word (@qwords) {
    if($title =~ /$word/i) {
      $found = 1;
      last;
    }
  }
  if(not $found) {
    push @non_title_matches, "$url $rank $title";
    next;
  }
  print_result($url, $rank, $title);
}
foreach my $entry (@non_title_matches) {
  my ($url, $rank, @title) = split(' ', $entry);
  print_result($url, $rank, "@title");
}
print "</ul>";
print "<center>";
#print "<a href='http://shrimp.stanford.edu/~taherh/ppr/'>Back</a>\n";
print "</center>";
print "</body></html>\n";

sub print_result {
  my ($url, $rank, $title) = @_;
  print "<li>$rank\t<a href=$url>" . $title . "</a><br>\n";
  print "&nbsp;&nbsp;<small>$url</small>&nbsp;<a href='getpage.pl?url=" . uri_escape("$url") . "'><small>cache</small></a><br>&nbsp;\n";
}
