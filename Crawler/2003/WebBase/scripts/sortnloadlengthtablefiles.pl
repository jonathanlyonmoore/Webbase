#!/usr/bin/perl -w
#
#sortnloadlengthtablefiles.pl - 
#      Iterate sort & loading lengthstable over the 100 buckets
#         no intermediate files
#      9/1 major changes by gsw:
#       Had to add filter at front to convert binary unsigned text
#          to the actual signed form
#       Also we now filter the 00 file for DocID=0
# Gary Wesley <gary@db.stanford.edu> 12/00
#
printf "Starting $0 using Perl %vd\n", $^V; #$PERL_VERSION;
my $startscripttime  = time();
my $scratchDir       = "/dfs/oh/1/tmp"; # best since run on
my $SORT             = "/bin/sort -u -T $scratchDir";
my $sourceFilePrefix = "/dfs/eh/5/tmp/gary/sizesAll"; # look this up in config later

#my $STRTCOMMAND      = "cat $sourceFilePrefix";
my $STRTCOMMAND      = "$SORT  out";
my $UNIQCMD           = " uniq --check-chars=20  ";
my $ENDCMD            = "| psql webbase \'-c COPY pagesizestable FROM stdin;\'";
#my $ENDCMD           = "  > outps01";

# Do the first one, and don't output 0 DocIDs
my $CONVERTER = " tosignednozero";
my $j       = "00";
my $COMMAND = "$STRTCOMMAND$j | $CONVERTER | $SORT | $UNIQCMD $ENDCMD";

#printf  localtime() . "\n". $COMMAND . "\n";
my $t0 = time();

system( $COMMAND );

#printf  localtime() . "\n";
my $t1 = time();
my $elapsed = $t1 - $t0;
print "00 took " . $elapsed  . " seconds or " . $elapsed/60 . " mins\n";


$CONVERTER  = "tosigned"; # only need to check first file for DOCID=0
foreach $i ( "01" .. "99" ) {
  $COMMAND = "$STRTCOMMAND$i | $CONVERTER | $SORT | $UNIQCMD $ENDCMD";
  #printf  localtime() . "start " . $i . " ". $COMMAND;
  $t0 = time();
  system( $COMMAND );
  $t1 = time();
  $elapsed = $t1 - $t0;
  print $i . " took " . $elapsed  . " seconds or " . $elapsed/60 . " mins\n";
}
  
printf  localtime() ;
print "Iterations finished.\n";
my $SQLCMD = "psql webbase -c 'SELECT COUNT(*) FROM pagesizestable;'";
system( $SQLCMD );
  
printf  localtime() . " end\n";
my $endscripttime =  time();
$elapsed = $endscripttime - $startscripttime;
print "script took " . $elapsed/60 . " mins or " . $elapsed/3600 . " hours\n";

exit 0;
