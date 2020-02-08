#!/usr/bin/perl -w
#
# diff.pl -- diff, but with pipe instead of file inputs
# Wang Lam <wlam@cs.stanford.edu>
# March 1999
#
# Usage: diff.pl command1 command2
# where command1 and command2 generate output to stdout for comparison
# Sadly, no diff switches supported at the moment...
#

die "Usage: diff.pl command1 command2\n" unless (scalar(@ARGV) == 2);

open(C1,"$ARGV[0] |") || die "Can't invoke $ARGV[0]: $!\n";
open(C2,"$ARGV[1] |") || die "Can't invoke $ARGV[1]: $!\n";

$line = 0;
$status = 0;

while(1) {
   $l = <C1>; $r = <C2>; ++$line;
   if (!defined($l)) {
      if (!defined($r)) {
         # We're done
         last;
      } else {
         print "Left stream ended prematurely\n";
         print "$line> $r";
         $status = 1;
         last;
      }
   } elsif (!defined($r)) {
      print "Right stream ended prematurely\n";
      print "$line< $l";
      $status = 1;
      last;
   }
   
   next if ($l eq $r);
   print "$line< $l";
   print "$line> $r";
   $status = 1;
}

close(C1);
close(C2);
exit $status;
