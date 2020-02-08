#!/usr/bin/perl -w
# Total how many bytes in a ls -l from std input

#   Gary Wesley <gary@db.stanford.edu> 5/03
require 5.003; # least we have tested on

#44949816(k) com.cisco.www
my $start = "";
my $tot;

while( chop($start = <STDIN>)){ # throws ignorable error on eof
    (my $bytes, my $fn) = split(/[\s\t]+/, $start); # parse line into tokens
    #printf $bytes . "\n";
    if( defined($bytes)) { $tot += $bytes; }
        
}
printf $tot . "   " . ( $tot / 1024 ) . "MB   " .  ( $tot / 1024 / 1024 ) . "GB\n";
exit 0;

