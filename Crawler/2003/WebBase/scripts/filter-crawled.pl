#!/usr/bin/perl -w
#
# filter-crawled.pl - filter out crawled sites from standard input
# Wang Lam <wlam@cs.stanford.edu> Mar 2002
#
# Intended typical use:  If siteserver dies, and loses track of the sites
# it had crawled, this script can be used to filter out crawled sites from
# sitenames before restarting siteserver.
#
# Usage:
# cd crawl_binaries/
# filtered-crawl.pl < sitenames.all > sitenames.smaller
# ln -sf sitenames.smaller sitenames
#
#

$CAT = "/bin/cat";

%sitedone = ();
$debug = 0;

# One of the following should be 1.
$done = 0;		# Filter out done sites.
$request = 1;		# Filter out started sites.

# Read the list of crawled sites from siteserver's log.
#
open(LOG,"$CAT site.log.??? |") || die "Can't read logs site.log.???: $!\n";
while(<LOG>) {
	if ($done) {
		# Extract sitenames listed as "Done:"
		next unless m#^\[\d\d:\d\d:\d\d \d\d/\d\d\] Done: (.*)$#;
		next if $1 =~ /^Starter \(\d+, Dummy\)$/;
		($sitename) = $1 =~ /^(.*) \(\d+, .*\)$/;
		print "Omit: $sitename\n" if $debug;
		$sitedone{$sitename} = 1;
	} elsif ($request) {
		# Extract sitenames listed for "Request:"
		next unless m#^\[\d\d:\d\d:\d\d \d\d/\d\d\] Request: (.*)$#;
		# Request: lines sometimes fail to have closing ')'! <shrug>
		($sitename) = $1 =~ /^(.*) \(.*$/;
		print "Omit: $sitename\n" if $debug;
		$sitedone{$sitename} = 1;
	}
}
close(LOG);

# Filter standard-input with %sitedone.
#
while(<STDIN>) {
	chomp;
	print "$_\n" unless $sitedone{$_};
}

