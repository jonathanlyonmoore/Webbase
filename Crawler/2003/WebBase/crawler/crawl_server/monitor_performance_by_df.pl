#!/usr/bin/perl -w
# display how many bytes written per minute by crawler
#  arg = minutes between samples
#   Gary Wesley <gary@db.stanford.edu> 12/02
require 5.003; # least we have tested on
my $sl;
if(@ARGV < 1) { 
    $sl = 60;
}
else {
  $sl = $ARGV[0];
}
if(@ARGV < 2) { 
    $loops = 60;
}
else {
  $loops = $ARGV[1];
}

open(STDERR, ">> errlog") || die $!;
#/dev/sda             1688891464 132050768 1539682552   8% /lfs/1
my $start;
my $i;
my $lfs1 = 1;
my $lfs2 = 0;
my $warnedhGB = 0;
my $warned50GB = 0;
my $warned20GB = 0;
my $warnedtGB = 0;
my $warnedGB = 0;
my $warnedhMB = 0;
#              
my $prevlfs1 =       12836; # how much on disk before crawl started
my $prevlfs2 =     210056;
my $percent_data =  0.9666;  # logs take a tiny bit
my $start1used;
for($i = 0; $i < $loops ; $i++){
    if( $lfs1 ){ 
	#system('perl status_all.pl | grep Sit');
	$start = `df /lfs/1 | tail -n 1`;
	
	(my $path,my $total, $start1used , my $remaining, my $stuff) = split(/[\s\t]+/, $start); # parse line into tokens
	
	if( $remaining < 100000  && !$warnedhMB ){ 
	    printf "DANGER !!!!!!!!!!!! < 100 MB left!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" . $remaining . " on " .  $path . "\n";$warnedhMB = 1;}
	
	else { if( $remaining < 1000000  && !$warnedGB){ 
	    printf "WARNING !!!!!!!!!!!! < 1 GB left!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" . $remaining . " on " .  $path . "\n";$warnedGB = 1;}
	       
	       else { if( $remaining < 10000000 && !$warnedtGB ){ 
		   printf "CAUTION !!!!!!!!!!!! < 10 GB left!!!!!!!!!!!!!!!!!!!!!!!!!!!" . $remaining . " on " .  $path . "\n";$warnedtGB = 1;}
		      else { if( $remaining < 20000000  && !$warned20GB){ 
			  printf "CAUTIONARY !!!!!!!!!!!! < 20 GB left!!!!!!" . $remaining . " on " .  $path . "\n";$warned20GB = 1;
		      }  
			     else { if( $remaining < 50000000  && !$warned50GB){ 
				 printf "CAUTIONARY !!!!!!!!!!!! < 50 GB left!!!!!!" . $remaining . " on " .  $path . "\n";$warned50GB = 1;
			     }   
				    else { if( $remaining < 100000000  && !$warnedhGB){ 
					printf "ADVISORY !!!!!!!!!!!! < 100 GB left!!!!!!" . $remaining . " on " .  $path . "\n";$warnedhGB = 1;}       
				       }
				}
			 }
		  }
	   }
    }
    my $start2used;

    if( $lfs2 ) {
	$start = `df | grep /lfs/2`;
	#/dev/md1             480763536  25310952 450568300   6% /lfs/2
	($path, undef, $start2used, $remaining, undef) = split(/[\s\t]+/, $start);
	
	if( $remaining < 100000  && !$warnedhMB ){ 
	    printf "DANGER !!!!!!!!!!!! < 100 MB left!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" . $remaining . " on " .  $path . "\n";$warnedhMB = 1;}
	
	else { if( $remaining < 1000000  && !$warnedGB){ 
	    printf "WARNING !!!!!!!!!!!! < 1 GB left!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" . $remaining . " on " .  $path . "\n";$warnedGB = 1;}
	       
	       else { if( $remaining < 10000000 && !$warnedtGB ){ 
		   printf "CAUTION !!!!!!!!!!!! < 10 GB left!!!!!!!!!!!!!!!!!!!!!!!!!!!" . $remaining . " on " .  $path . "\n";$warnedtGB = 1;}
		      
		      else { if( $remaining < 100000000  && !$warnedhGB){ 
			  printf "ADVISORY !!!!!!!!!!!! < 100 GB left!!!!!!" . $remaining . " on " .  $path . "\n";$warnedhGB = 1;}
			     
			 }
		  }
	   }
    }
    
    my  $onerawused ;
    my $oneused;my $end1used;
    sleep $sl * 60;
    if ( $lfs1 ) {
	my $end =  `df /lfs/1 | tail -n 1`;
	( undef , undef , $end1used, undef ) = split(/\s+/, $end); 
	$onerawused = ( $end1used -  $start1used ) ; 
	$oneused =  $onerawused * $percent_data; # logs take a tiny bit
    }
    else { $oneused = 0; $end1used = 0; $onerawused = 0; $prevlfs1 = 0 ;}
    my $tworawused;my $twoused; 
    if ( $lfs2 ) {
        #/dev/md1             480763536  24384072 451495180   6% /lfs/2
	$end = `df | grep /lfs/2`; 
	(undef , undef,    $end2used, undef) = split(/\s+/, $end); 
	$tworawused = ( $end2used -  $start2used ) ; 
	$twoused = $tworawused * $percent_data; # logs take a tiny bit
    }
    else { $twoused = 0; $end2used = 0; $tworawused = 0 ; $prevlfs2 = 0;}
    my $used = $twoused + $oneused;
    
    #printf " $onerawused  $end1used   two:$tworawused $end2used  $start2used  $used \n";
    #printf " $used $end1used two: $end2used prev1 $prevlfs1 prev2 $prevlfs2 \n";
    #printf "    " . $used/1024 . "MB in " .  $sl . " minutes\n";
    my $mbmin =  ( $used / 1024 ) /  $sl  ;
    my $gbwritten = ((( $end1used + $end2used ) - ( $prevlfs1 + $prevlfs2)) /1024 /1024 ) * $percent_data;# allow for logs
    #my $gbwritten = ((( $end2used  ) - ( $prevlfs2 )) /1024 /1024 ) * $percent_data;# allow for logs
    my $mbhr =  $mbmin * 60;
    my $gbhr =  $mbhr / 1024;
    my $gbday = ( $mbhr * 24 ) / 1024;
    my $gbmonth = $gbday * 30;
    my $tbmonth = $gbmonth / 1024;
    $tod =  `date +%H`;
    if( $tod == "00"){printf  `date +%D` ;}
    printf "%5.3f GB : %4.2f GB/hr %3.0f GB/day  %2.2f TB/mo cmprsd ",
                   $gbwritten,  $gbhr ,   $gbday ,    $tbmonth;
    my $line = `ps -eaf | grep "crawl_server -id " | wc -l`;
    #printf "\n";
    if( $sl > 9 ) { system('perl sitetot.pl');}
    printf `date +%H:%M`;
}
exit 0;
