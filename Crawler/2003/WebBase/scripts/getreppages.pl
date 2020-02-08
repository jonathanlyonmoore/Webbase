#!/usr/bin/perl -w
#
# client to access repository page feeder
#
#  optional args:  offset, num-pages
#
# get from repository
#
#   Gary Wesley <gary@db.stanford.edu> 5/01
#
use strict;
#use diagnostics;

require 5.005; # least we have tested on

printf "Starting $0 using Perl  %vd\n", $^V ;

my $logFile  = "getpagesreplog2"; 
my $EOL = "\015\012";

my $offset = 0;
my $pages  = 0;
# Find the path where the WebBase code tree begins.
use FindBin;
$_ = $FindBin::Bin;  # happens to end without a slash
s#/([^/]+)$#/# ;
my $WEBBASE = $_;   # happens to end in a slash

if(@ARGV < 1) { 
  printf("Enter offset: ");
  chop($_ = <STDIN>);
  die "Non-numeric, try again..." unless /\d/;
  $offset = $_;
  printf "Offset $offset requested\n";
}
else {
  $offset = $ARGV[0];
}

if(@ARGV < 2) {
  printf("Enter number of pages required: ");
  chop($_ = <STDIN>);
  die "Non-numeric, try again..." unless /\d/;
  $pages = $_;
  printf "$pages pages requested\n";
}
else {
  $pages = $ARGV[1];
}


# Let them go ahead and run if they want
my $commandLine = "$WEBBASE/bin/RunHandlers $WEBBASE/inputs/webbase.conf \"rep://dummyhost/../inputs/rep2001?compress=0&offset=$offset&numPages=$pages\";
# > $logFile";
printf("Do you want to run $commandLine now?(Y/N): ");
chop($_ = <STDIN>);
die "You must answer Y or N, try again..." unless /\w/;
my  $runit = $_;

if(/Y|YES/i){  
    
  # make sure . command will work on chub
  # older Linux/Perl requires . command somehow tcsh?
  chdir  "$WEBBASE/handlers" || die "chdir: $!";  

  # start processing pages
  
  print "Issuing command: $commandLine at " . localtime() . "\n";
  system("$commandLine");
  
  my $exit_value = $? >> 8;
  if($exit_value > 0){ print "Exited with a bad: $exit_value\n"; }
  print "Done at " . localtime() . "\n";
}

exit 0;
