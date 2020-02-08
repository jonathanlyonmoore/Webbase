#!/usr/bin/perl -w
#
# linklookup.pl - looks up forward or backward link information from WebBase
#
# Takes URLs and finds the pages it has anchors to (forward links), 
# or pages that link to it (backward links).
#
# Wang Lam <wlam@cs.stanford.edu>
# 14 Jan 2000
#
# Updated 16 Feb 2001 to use Perl::DBD; this change is still untested.
# Updated 6 Mar 2000 to use PostgreSQL and webbase.conf
# Updated 19 Sep 2002 to follow hashlookup/ binaries from hashlookup/ to bin/
#
# Usage: linklookup.pl {forward|backward} webbase.conf
# Feed one URL per line on standard input, receive on output for each URL,
# left-aligned (on success):
#       Links: (number of links found)
#	(link1)
#	(link2)
#	:
#	(linkn)
# Link information may contain the same URL more than once, but
# duplicates are not removed in the output of this program.
# Input another URL to repeat.  Program exits on EOF.
# On failure, stderr displays an error message before the program quits.
#
#

use DBI;

$| = 1;

# Find the path where all the supporting WebBase commands are.
use FindBin;
$_ = $FindBin::Bin;  # happens to end without a slash
s#/([^/]+)$#/# ;
$WEBBASEROOT = $_;   # happens to end in a slash

$UNIQ = 1;  # Throw out duplicate links (currently implemented by dubious hack)
            # (i.e., link information happens to come sorted...)
$DEBUG = 1;

# Forward or backward?
if (scalar(@ARGV)==2 and $ARGV[0] eq "forward") {
   $DIRECTION = "forward";
} elsif (scalar(@ARGV)==2 and $ARGV[0] eq "backward") {
   $DIRECTION = "backward";
} else {
   die "Usage: linklookup.pl {forward|backward} webbase.conf\nfor forward/backward links";
}
shift;

# Old way
# $FORWARDLINKSDIR = "/dfs/pita/14/taherh/links";
# $BACKLINKSDIR = "/dfs/pita/14/taherh/blinks";
# $HASHLOOKUPDIRS = "/dfs/pita/13/wlam/deltaindex/urlindex%02d /dfs/pita/13/wlam/deltafile%d";
#
# New way
require "$WEBBASEROOT/utils/confloader.pl";
&ConfLoader::loadValues(shift(@ARGV));
$FORWARDLINKSDIR = &ConfLoader::getValue('FLINKS_DIR');
$BACKLINKSDIR = &ConfLoader::getValue('BLINKS_DIR');
$HASHLOOKUPDIRS = &ConfLoader::getValue('DELTA_URLS_DIR')."/urlindex%03d.s ".&ConfLoader::getValue('DELTA_URLS_DIR')."/deltafile%03d";

while (<>) {
   # Get the URL to process.
   $sourceURL = $_;
   chomp $sourceURL;

   # Normalize the URL, if possible
   $normalized = `$WEBBASEROOT/bin/normalizer 2>1 <<EOF\
$sourceURL\
EOF`;
   if ( $normalized eq '' ) {
      warn "*** Cannot normalize URL\n";
      # Code will continue to run, attempting to look up unnormalized URL.
   } else {
      print "Normalized URL: $normalized" if $DEBUG;
      $sourceURL = $normalized;
   }

   # Turn the URL into a hash
   $sourceDocIDdec = `$WEBBASEROOT/bin/hash <<EOF | head -1\
$sourceURL\
EOF` ;
   chomp $sourceDocIDdec;  # The hash (decimal text)
$sourceDocIDhex = `$WEBBASEROOT/bin/hash hex <<EOF | head -1\
$sourceURL\
EOF` ;
   chomp $sourceDocIDhex;  # The hash (hexadecimal text, no "0x" prefix)
   $sourceDocIDneg = `$WEBBASEROOT/bin/hash <<EOF | head -1 | $WEBBASEROOT/bin/neg\
$sourceURL\
EOF` ;
   chomp $sourceDocIDneg;  # The hash (signed decimal text)
                           # (this is needed for the broken C query engine)
   print STDERR "$sourceURL -> $sourceDocIDdec $sourceDocIDneg 0x$sourceDocIDhex\n" if $DEBUG;

   # Compute the 00-63 index of the hash
   # Take only the upper six bits of the highest eight bits in the 64-bit word. 
   $_ = $sourceDocIDhex;
   $_ = "0$_" while (length $_ < 16);  # get to 64 bits
   $_ = substr($_,0,2);
   $_ = ((hex $_) >> 2);  
   $sourceDocIDindex = (length $_ == 2) ? $_ : "0$_" ;
   print STDERR "Index: $sourceDocIDindex\n" if $DEBUG;

   # Query the hash
 if ($MYSQL) {
   $table = ($DIRECTION eq 'forward') ? 
            'ForwardLinkOffsets' : 
            "BackwardLinkOffsets$sourceDocIDindex";
   $field = ($DIRECTION eq 'forward') ? 'DocId' : 'DocID';
   $query = "SELECT * FROM $table WHERE $field = $sourceDocIDneg";
 } else {  # PostgreSQL
   $query = "SELECT * FROM ${DIRECTION}linkoffsets WHERE DocID = $sourceDocIDdec";
 }
   print STDERR "Query: $query\n" if $DEBUG;
# if ($MYSQL) {
#   $line = `echo '$query' | $WEBBASEROOT/mysql/fetch 2> /dev/null | tail -1`;
# } else {  # PostgreSQL
#   $line = `echo '$query' | $WEBBASEROOT/postgresql/fetch 2> /dev/null | tail -1`;
# }
#    chomp $line;
#    print STDERR "Result: $line\n" if $DEBUG;
#    die "*** Cannot find any references for given URL\n" unless $line;
# 
#    # Extract the offset
#    $_ = $line;
#    ($id, $offset) = split;
#    # print STDERR "ID for matching: $id\n" if $DEBUG;
#    die "*** Internal error: offset information is not for desired URL\n"
#       unless (($id eq $sourceDocIDdec) || ($id eq $sourceDocIDneg));  # PostgreSQL actually returns the neg.  :P
 if ($MYSQL) {
   die "*** MySQL is no longer supported in linklookup.pl\n";
   # It can borrow the DBD code below anyway.
 } else { # PostgreSQL
   my $dbh = DBI->connect('DBI:Pg:WebBase') ||
      die "*** Cannot connect to WebBase internal database: ". DBI->errstr;
   my $sth = $dbh->prepare($query) ||
      die "*** Cannot create query for internal database: ". $dbh->errstr;
   $sth->execute() ||
      die "*** Cannot execute query for internal database: ". $sth->errstr;
   $offset = ($sth->fetchrow_array())[0];
   print STDERR "DBD-fetched offset: $offset\n" if $DEBUG;
 }

   # Look up the offset
   $file = ($DIRECTION eq 'forward') ?
           "$FORWARDLINKSDIR/links$sourceDocIDindex.su" :
           "$BACKLINKSDIR/blinks$sourceDocIDindex.s.c" ;
   open(EDGES,"< $file") || die "*** Can't open $file: $!\n";
   seek(EDGES,$offset,0);
   
   # Read the file at the offset
   # DocID
   read(EDGES,$junk,8);  # Should sprintf into $id
   ### This verification step skipped for lack of time.  :(
   # die "*** Internal error: DocID mismatch in adjacency list.\n"
   #    if (unpack("Q",$junk) != $sourceDocID);
   # In-degree
   read(EDGES,$junk,4);  # Since it's least significant first, and so are we...
   $numlinks = unpack("V",$junk);
   read(EDGES,$junk,4);  # Hope the upper four bytes are zero!
   if (unpack("V",$junk) != 0) {
      die "*** Internal error: Cannot handle more than 2^32 - 1 links!\n";
   }
   # Links
   @links = ("placeholder");
   $savedlinks = 0;
   foreach $_ ( 1 .. $numlinks ) {
      # Read all eight bytes
      read(EDGES,$s1,4);  # least-significant-quad
      read(EDGES,$s2,4);  # most-significant-quad
      # Turn it into a hex string
      $junk = sprintf("%08x%08x",unpack("V",$s2),unpack("V",$s1));
      # print STDERR "Link $_: $junk\n" if $DEBUG;
      # Save the hex string
      $oldval = pop(@links);
      push(@links,$oldval);
      if (($junk ne $oldval) || !$UNIQ) {
         push(@links,$junk);
         ++$savedlinks;
      } 
   }
   shift(@links);  # Remove "placeholder" we defined in.
   print "Links: $savedlinks\n";  # Actual output to the user!  :)
   # Turn the hex strings into URLs on stdout
   open(LOOKUP,"| $WEBBASEROOT/bin/hashlookup $HASHLOOKUPDIRS hex")
      || die "*** Can't run hashlookup: $!\n";
   print LOOKUP join("\n",@links);
   close(LOOKUP);
   close(EDGES); 
}
