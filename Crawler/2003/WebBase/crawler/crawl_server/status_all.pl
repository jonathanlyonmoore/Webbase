#!/usr/bin/perl -w
# status all crawlers
#
#   Gary Wesley <gary@db.stanford.edu> 4/03
require 5.005; # least we have tested on
my $x;
for($x = 0; $x < 8; $x += 1 ){
    #print "# " .  $x . ": ";
     print `perl ../../scripts/crawl_cookedstatus.pl $x` ;
}

exit 0;
