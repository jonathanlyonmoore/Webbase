#!/usr/bin/perl -w
#
# hashlookup.pl - looks up URL from WebBase URL ID
#
# Takes hash values and finds its corresponding URL.
# (Wrapper for hashlookup/hashlookup to use webbase.conf.)
#
# Wang Lam <wlam@cs.stanford.edu>
# 28 Sep 2000
#
# Usage: hashlookup.pl webbase.conf [hex]
# Feed one URL ID per line on standard input, as decimal text or hexadecimal
# text (if 'hex' specified), and receive on output each corresponding URL, 
# left-aligned (on success):
#	(URL1)
#	(URL2)
#	:
# Program exits on EOF.
# On failure, stderr displays an error message before the program quits.
#
#

$| = 1;

# Find the path where all the supporting WebBase commands are.
use FindBin;
$_ = $FindBin::Bin;  # happens to end without a slash
s#/([^/]+)$#/# ;
$WEBBASEROOT = $_;   # happens to end in a slash

if (scalar(@ARGV) < 1) {
   die "Usage: hashlookup.pl webbase.conf\nfor URLs of WebBase URL IDs";
}
$HEX = ((scalar(@ARGV)==2) && ($ARGV[1] eq 'hex')) ? 'hex' : '';

# Find the path of the hashlookup index files.
require "$WEBBASEROOT/utils/confloader.pl";
&ConfLoader::loadValues(shift(@ARGV));
$HASHLOOKUPDIRS = &ConfLoader::getValue('DELTA_URLS_DIR')."/urlindex%03d.s ".&ConfLoader::getValue('DELTA_URLS_DIR')."/deltafile%03d";

open(LOOKUP,"| $WEBBASEROOT/bin/hashlookup $HASHLOOKUPDIRS $HEX")
   || die "*** Can't run hashlookup: $!\n";
select(LOOKUP);
$| = 1;

while (<>) {
   # Turn the hex strings into URLs on stdout
   print LOOKUP $_;
}
close(LOOKUP);

