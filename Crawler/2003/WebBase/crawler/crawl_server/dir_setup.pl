#!/usr/bin/perl -w
# set up some config files for individual crawlers
#  copied from config/dir.config.temp
#
#   Gary Wesley <gary@db.stanford.edu> 6/03
require 5.005; # least we have tested on
die "Usage: dir_setup.pl start end\n" unless defined $ARGV[0];
my $start = $ARGV[0];
my $end   = $ARGV[1]; 
my $command;
for($x = $start; $x < $end; $x += 1 ){
   $command = "cp config/dir.config.temp  config/dir.config." . $x;
   system( $command );
}



exit 0;
