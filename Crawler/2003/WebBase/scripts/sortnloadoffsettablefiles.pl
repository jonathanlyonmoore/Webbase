#!/usr/bin/perl -w
#
#sortnloadoffsettablefiles.pl - 
#      Iterate sort & loading offsetstable over the 10 buckets
#         no intermediate files
# Gary Wesley <gary@db.stanford.edu> 9/1
#
printf "Starting $0 using Perl %vd\n", $^V; #$PERL_VERSION;
my $startscripttime  = time();
my $scratchDir       = "/lfs/0/tmp";
my $SORT             = "/bin/sort -u -T $scratchDir";
my $sourceFilePrefix = "/dfs/eh/0/tmp/gary/offsetoct/offsets"; # look this up in config later
my $SQLCMD = "psql webbase -c 'SELECT COUNT(*) FROM  offsetlookup;'";

my $STRTCOMMAND      = "cat $sourceFilePrefix";
#my $STRTCOMMAND      = "$SORT  out";
my $UNIQCMD           = " uniq --check-chars=20  ";
my $ENDCMD            = "| psql webbase -U webbase \'-c COPY offsetlookup FROM stdin;\'";
#my $ENDCMD           = "  > outoff0";

###########################################################################
# Do the first one, and don't output 0 DocIDs
my $CONVERTER = "tosignednoboth";
my $j       = "00";
my $COMMAND = "$STRTCOMMAND$j | $CONVERTER  | $SORT | $UNIQCMD $ENDCMD";

printf  localtime() . "\n". $COMMAND . "\n";
my $t0 = time();

system( $COMMAND );

printf  localtime() . "\n";
my $t1 = time();
my $elapsed = $t1 - $t0;
print "0 took " . $elapsed  . " seconds or " . $elapsed/60 . " mins\n";
###########################################################################

#system( $SQLCMD );
printf  localtime() . "\n";



$CONVERTER  = "tosigned"; # only need to check first file for DOCID=0
foreach $i ( "01" .. "31" ) {
  $COMMAND = "$STRTCOMMAND$i | $CONVERTER | $SORT | $UNIQCMD $ENDCMD";
  #printf  localtime() . "start " . $i . " ". $COMMAND;
  $t0 = time();
  system( $COMMAND );
  $t1 = time();
  $elapsed = $t1 - $t0;
  print " file " . $i . " took " . $elapsed  . " seconds or " . $elapsed/60 . " mins\n";
  #system( $SQLCMD );
}
  
printf  localtime() ;
print "Iterations finished.\n";

#system( $SQLCMD );
  
printf  localtime() . " end\n";
my $endscripttime =  time();
$elapsed = $endscripttime - $startscripttime;
print "script took " . $elapsed/60 . " mins or " . $elapsed/3600 . " hours\n";

exit 0;
