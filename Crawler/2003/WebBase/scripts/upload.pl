#!/usr/bin/perl -w
#
#upload.pl
#   upload a list of files from stdin to SDSC
# usage: ls <whatever> | upload.pl

# Gary Wesley <gary@db.stanford.edu> 10/1
#    size verification added 12/2
#
printf "Starting $0 using Perl %vd---\n", $^V; #$PERL_VERSION;
my $COMMAND        = ""; 
my $sourceFileName = "";
my $outline        = "";
my $elapsed = 0;
my $t0      = 0;
my $destDir = "";
use File::Basename;
use File::stat;

while( chop($_ = <STDIN>)){ # throws ignorable error on eof
  
  $sourceFileName = $_;
  $COMMAND = "ls -sh "  .  $sourceFileName ;
  system( $COMMAND );

  #$COMMAND = "Sput " .  $sourceFileName . " \.";
  #
  # strip path away for destination
  # otherwise SRB will make a new subdirectory automatically
  #
  ($base,$path,$type) = fileparse($sourceFileName, '/*');
  #  my $dummy = "$path $type";
  $COMMAND = "Sput " .  $sourceFileName . " $destDir" . "$base";
  
  #printf "\"$COMMAND\" requested\n";  
  $t0 = time();
  
  system( $COMMAND ); # actual put
 
###########################################################################
  #verify file size
###########################################################################
  $localFileStats = stat(  $sourceFileName );
  #printf "Local file size     %s\n",$localFileStats->size ;
  
  $COMMAND = "Sls -l " .  $sourceFileName;
  chomp($Sls = `$COMMAND`);
  ( undef, undef, undef, undef, $remoteFileSize) = split / +/, $Sls;
  #printf "Remote file size:  %d\n", $remoteFileSize;
  if( $remoteFileSize > 0 ){ # prevent Sls error messing all up
    if( $remoteFileSize != $localFileStats->size){
      if($localFileStats->size  > 0 ){
	printf ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>   Size mismatch, re-upload " .  $sourceFileName . " size: " . $localFileStats->size . " local vs " . $remoteFileSize . " remote\n";
	$COMMAND = "Srm $sourceFileName";
	#system( $COMMAND );  
	printf ">>>>$fileName wrong size file, erase for reupload\n";
      }
    }
    else{ printf "$sourceFileName @ correct size " . $remoteFileSize. "\n"; }
  }
#################################################################################
  
  $elapsed = time() - $t0; # in seconds
  if( $elapsed > 4){ # Do not time an error like "already exists"
    $outline = sprintf  "took %d secs or %9.2f mins at ", $elapsed, $elapsed/60;
    print $outline .  localtime() . "\n" ;
  }
}
exit 0;
