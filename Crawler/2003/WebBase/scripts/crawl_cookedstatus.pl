#!/usr/bin/perl -w
#
# Cooked crawler-status reports
# Wang Lam <wlam@cs.stanford.edu>
# May 2003
#
#

die "Usage: crawl_cookedstatus.pl crawler-id\n" unless scalar(@ARGV) > 0;
$id = shift @ARGV;

open(STAT,"crawl_ctl.pl status $id |") || die "Can't run crawl_ctl.pl: $!\n";

while(<STAT>) {
   print "Crawler $id: $_";
   $starttime = $1 if /^Start seconds (\d+)$/;
   $endtime = $1 if /^Status seconds (\d+)$/;
   $bytes = $1 if /^Bytes saved (\d+)$/;
   $pages = $1 if /^Pages saved (\d+)$/;
}
close(STAT);

print "Crawler $id: Run time ",$endtime - $starttime,"\n";
if ($endtime - $starttime == 0) {
   $bytes = 0;  $pages = 0;
   $endtime = $starttime + 1;
}
print "Crawler $id: Bytes per second ",$bytes/($endtime - $starttime),"\n";
print "Crawler $id: Pages per second ",$pages/($endtime - $starttime),"\n";

