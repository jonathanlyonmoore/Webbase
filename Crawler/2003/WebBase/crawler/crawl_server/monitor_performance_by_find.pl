#!/usr/bin/perl -w
# display how many bytes written per minute by crawler
#  arg = minutes between samples
# NOTE: this is only practical when < 100GB in the path
#   more accurate than monitor_performance_by_df.pl
#   Gary Wesley <gary@db.stanford.edu> 08/03
require 5.003; # least we have tested on

if(@ARGV < 1) { 
    $data =  "data";
}
else {
  $data = $ARGV[0];
}

if(@ARGV < 2) { 
    $sleepTime = 10;
}
else {
  $sleepTime = $ARGV[1];
}
if(@ARGV < 3) { 
    $loops = 60;
}
else {
  $loops = $ARGV[2];
}

# a lot of spurious errors come out otherwise
open(STDERR, ">> log/errlog") || die $!; 
#/dev/sda             1688891464 132050768 1539682552   8% /lfs/1
my $start;
my $i;

my $warned100GB = 0;
my $warned50GB = 0;
my $warned20GB = 0;
my $warned10GB = 0;
my $warnedGB = 0;
my $warned100MB = 0;

my $logsFraction =  1;  # only counts rep!
#my $logsFraction =  0.9606;  # logs take a tiny bit
for($i = 0; $i < $loops ; $i++){
    #system('perl status_all.pl | grep Sit');
   
    $start = `df /lfs/1 | tail -n 1`;
    
    (my $path,my $total, undef, my $remaining, undef) = split(/[\s\t]+/, $start); # parse line into tokens
   
    if( $remaining < 100000  && !$warned100MB ){ 
	printf "DANGER !!!!!!!!!!!! < 100 MB left!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" . $remaining . " on " .  $path . "\n";$warned100MB = 1;}
    
    else { if( $remaining < 1000000  && !$warnedGB){ 
	printf "WARNING !!!!!!!!!!!! < 1 GB left!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" . $remaining . " on " .  $path . "\n";$warnedGB = 1;$warned10GB = 1;$warned20GB = 1;$warned50GB = 1;$warned100GB = 1;}
	   
	   else { if( $remaining < 10000000 && !$warned10GB ){ 
	       printf "CAUTION !!!!!!!!!!!! < 10 GB left!!!!!!!!!!!!!!!!!!!!!!!!!!!" . $remaining . " on " .  $path . "\n";$warned10GB = 1;$warned20GB = 1;$warned50GB = 1;$warned100GB = 1;}
		  else { if( $remaining < 20000000  && !$warned20GB){ 
		      printf "CAUTIONARY !!!!!!!!!!!! < 20 GB left!!!!!!" . $remaining . " on " .  $path . "\n";$warned20GB = 1;$warned50GB = 1;$warned100GB = 1;
		  }  
			 else { if( $remaining < 50000000  && !$warned50GB){ 
			     printf "CAUTIONGARY !!!!!!!!!!!! < 50 GB left!!!!!!" . $remaining . " on " .  $path . "\n";$warned50GB = 1;$warned100GB = 1;
			 }   
				else { if( $remaining < 100000000  && !$warned100GB){ 
				    printf "ADVISORY !!!!!!!!!!!! < 100 GB left!!!!!!" . $remaining . " on " .  $path . "\n";$warned100GB = 1;}       
				   }
			    }
		     }
	      }
       }
    #printf "find $data -type d | egrep \'_repository_/text\$\' | xargs du -s --block-size=1024 | ~webbase/dli2/src/WebBase/crawler/remote_crawl_binaries//leaftot.pl\n";
    $start = ` find $data -type d | egrep \'_repository_/text\$\' | xargs du -s --block-size=1024 | leaftot.pl` ;
    (my  $start1used, my $mbs , my $gbs )  = split(/[\s\t]+/, $start); # parse line into tokens
    
    sleep $sleepTime * 60;

     my $end  = ` find $data -type d | egrep \'_repository_/text\$\' | xargs du -s --block-size=1024 | leaftot.pl` ;
    (my  $end1used, my $mbe , my $gbe )  = split(/[\s\t]+/, $end); # parse line into tokens

    my $onerawused = ( $end1used -  $start1used ) ; 
    my $oneused =  $onerawused * $logsFraction;
    
    my $used = $oneused;
        
    #printf "$end1used minus  $start1used equals $used \n";
    
    #printf "    " . $used/1024 . "MB in " .  $sleepTime . " minutes\n";
    my $mbmin =  ( $used / 1024 ) /  $sleepTime  ;

    my $gbwritten = (   $end1used    /1024 /1024 );# * $logsFraction;# allow for logs

    my $mbhr =  $mbmin * 60;
    my $gbhr =  $mbhr / 1024;
    my $gbday = ( $mbhr * 24 ) / 1024;
    my $gbmonth = $gbday * 30;
    my $tbmonth = $gbmonth / 1024;
    $tod =  `date +%H`;
    if( $tod == "00"){printf  `date +%D` ;}
    printf "%5.6f GB : %5.3f MB/hr %4.6f GB/hr %3.3f GB/day  %2.6f TB/mo cmprsd ",
    $gbwritten,   $mbhr, $gbhr ,   $gbday ,    $tbmonth;

    #my $line = `ps -eaf | grep "crawl_server -id " | wc -l`;
    #system('perl sitetot.pl 0 19 ');
    printf `date +%H:%M`;
}
exit 0;
