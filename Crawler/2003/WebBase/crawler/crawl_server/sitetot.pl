#!/usr/bin/perl -w
# total number of threads in all crawlers x-y
#   Gary Wesley <gary@db.stanford.edu> 12/02
require 5.003; # least we have tested on
my $sl;
if(@ARGV < 1) { 
    $begin = 0;
}
else {
  $begin = $ARGV[0];
}
if(@ARGV < 2) { 
    $end = 20;
}
else {
  $end = $ARGV[1];
}


my $start;
my $i;
my $j = 0;
my $c ;
my $id;
my $label;
my $num;

my $cmd;
for($j = $begin; $j <= $end; $j++){
  $cmd = "perl ../../scripts/crawl_cookedstatus.pl $j  | grep Site";
  $start = `$cmd`;
  #printf $start;
  #Crawler 1 : Sites 20
  $c = "";
  if( $start =~ m/Crawler/ ){
    ($c ,$id, undef, $label, $num) = split(/[\s\t]+/, $start); # parse line into tokens
    $tot +=  $num;
  }
}
printf "$tot Threads @ " ;`date +%H`;

exit $tot;

