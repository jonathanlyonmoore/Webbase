#!/usr/bin/perl -w
#
# addHandler.pl   integrate a new handler
#
# Author: Gary Wesley <gary@db.stanford.edu> 4/01
#
# files used: file name = name of source file for handler to be added 
#                         w/o extension
#             helper file name = name of source file for helper to be added 
#                         w/o extension
#             all_handlers.h, Makefile, webbase.conf
# also used: class name of handler for a new() in all_handlers
#
# steps:
#       0) write reference versions to baseline/ if not present
#          these will be used to build new version and as an
#          effective backoff - by always being source
#       1) prompt for required handler source file, remove any extension
#       2) prompt for optional source file for helper, remove any extension
#       3) prompt for required handler class name
#       4) save off existing Makefile as Makefileo
#       5) add to  handler object file and helper object file to
#          Makefile OBJS = line
#       6) save off existing all_handlers.h as all_handlers.ho
#       7) add new(class) to all_handlers.h pushback line
#          add include to all_handlers.h includes
#       8) save off existing webbase.conf as webbase.confo
#       9) add output dir and ON flag to webbase.conf
#      10) optionally do gmake
#
# notes: can be ran several times, fresh copies will be used each time
#          so only the last run will have a lasting effect
#        we guard against them entering an extension and remove it
#        we obviously care nothing for efficiency here
#        do NOT reverse helper class and handler class filenames
#        if you futz around inside this script or in the source files
#          it's your funeral!
printf "Starting $0 using Perl %vd\n", $^V; #$PERL_VERSION;
require v5.0; # perl 5 or greater
use strict;

# Find the path where the WebBase code tree begins.
use FindBin;
$_ = $FindBin::Bin;  # happens to end without a slash
s#/([^/]+)$#/# ;
my $WEBBASEROOT = $_;   # happens to end in a slash

chdir  "$WEBBASEROOT/handlers" || die "run from handlers or scripts dir: $!";

my $MakefileIn  = "$WEBBASEROOT/handlers/baseline/Makefile";
my $MakefileOut = "$WEBBASEROOT/Makefile";
my $MakefileO   = "$WEBBASEROOT/Makefileo";        # not really used afterwards
my $allFileIn   = "$WEBBASEROOT/handlers/baseline/all_handlers.h";
my $allFileOut  = "$WEBBASEROOT/handlers/all_handlers.h";
my $allFileO    = "$WEBBASEROOT/handlers/all_handlers.ho";  # not really used afterwards
my $configFileIn  = "$WEBBASEROOT/handlers/baseline/webbase.conf";
my $configFileOut = "$WEBBASEROOT/inputs/webbase.conf"; 
my $configFileO   = "$WEBBASEROOT/inputs/webbase.confo";  # not really used after

printf("Is this a retry of a handler install?(Y/N): ");
chop($_ = <STDIN>);
#die "You must answer Y or N, try again..." unless /\w/;
my  $retry = $_;


if(/Y|YES/i){
  printf "Retry new handler addition\n";
  
  # ensure baselines exist, put a copy there if not
  # This should only be necessary on the first run.
  # plain, readable, text file
  my @args =( "cp", "$MakefileOut", "$MakefileIn" );
  (-f $MakefileIn && -R $MakefileIn && -T $MakefileIn) || system(@args );
  
  @args =( "cp", "$allFileOut", "$allFileIn" );
  (-f $allFileIn && -R $allFileIn && -T $allFileIn) || system(@args );
  
  @args =( "cp", "$configFileOut", "$configFileIn" );
  (-f $configFileIn && -R $configFileIn && -T $configFileIn) || system(@args );
}
else{
  printf "Adding a new handler\n";
  
  # renew baselines with latest  version 
  # this enables us to add additonal handlers 
  # original is still available as baseline/<x>O
  # note that this is only done the first time around,
  # in later runs, the file exists and is not written over
  my @args =( "cp", "$MakefileOut", "$MakefileO" );
  (-f $MakefileO && -R $MakefileO && -T $MakefileO) || system(@args );

  @args =( "cp", "$MakefileOut", "$MakefileIn" );
  system(@args );
  
  @args    =( "cp", "$allFileOut",  "$allFileO" );
  (-f $allFileO  && -R $allFileO  && -T $allFileO ) || system(@args );

  @args    =( "cp", "$allFileOut",  "$allFileIn" );
  system(@args ); 

  @args    =( "cp", "$configFileOut",  "$configFileO" );
  system(@args );

  @args    =( "cp", "$configFileOut",  "$configFileIn" );
  system(@args ); 
}


# to add to Makefile OBJS= line
# to add to all-handlers.h includes
printf("Enter the file name for the new handler source without extension: ");
chop($_ = <STDIN>);
die "No file name entered, try again...\n" unless /\w/;

my $sourceFileName = $_;
if( /(.+?)\.(\w)/){ 
  $sourceFileName = $1;  # the stuff before a .
  printf("removing file extension $2 giving       ");
}
printf("file name = $sourceFileName\n");
my $pat = "$WEBBASEROOT/handlers/$sourceFileName.cc";
die "File $pat does not exist...\n" unless -f $pat && -T $pat ;


# to add to Makefile OBJS = line

printf("Enter the file name of any helper source for your main class (eg an output writer) (cr for none): ");
my      $sourceFileNameHelper;
chop($_ = <STDIN>);

if( /\w/ ){    # if something (wordish) was entered
  $sourceFileNameHelper = $_;
  if(   $sourceFileNameHelper =~ /(.+?)\.(\w)/){ 
    $sourceFileNameHelper = $1; # the stuff before a .
    printf("removing file extension $2  ");  
    printf("giving: $sourceFileNameHelper\n");
  }
  printf("helper = $sourceFileNameHelper\n");
  $pat = "$WEBBASEROOT/handlers/$sourceFileNameHelper.cc";
  die "File $pat does not exist...\n" unless -f $pat && -T $pat ;
}
else{
  warn "No helper entered, enter ^C to stop";
  $sourceFileNameHelper = "";
};



# to add to all-handlers.h pushback line
printf("Enter the class name for the new handler: ");
chop($_ = <STDIN>);
die "No class name entered, try again...\n" unless /\w/;
my                   $className = $_;
printf("class name = $className\n");


# to add path to webbase.conf
# to add turnon flag too
printf("Enter the output file path: ");
chop($_ = <STDIN>);
die "No path entered, try again...\n" unless /\w/;
my $path = $_;

printf("output path = $path\n");

my   $linkHandler;
printf("Do you implement LINKHANDLER interface? (Y/N): ");
chop($_ = <STDIN>);
if(/Y|YES/i){
      $linkHandler = 1; }
else{ $linkHandler = 0; }


# to add to Makefile flags line

my   $compileFlags;
printf("Are you using any of the urlHash classes we furnished (eg url2Hash)? (Y/N): ");
chop($_ = <STDIN>);
#die "You must answer Y or N, try again..." unless /\w/;
if(/Y|YES/i){
  $compileFlags = "-IurlHash";
}


printf("Are you using any of the text normalization classes we furnished? (Y/N): ");
chop($_ = <STDIN>);
#die "You must answer Y or N, try again..." unless /\w/;
if(/Y|YES/i){
  $compileFlags = "$compileFlags -I\$\(INCLWWW\)";
}

my   $moCompileFlags;
printf("Enter any other compiler flags needed (eg -O3 ) (cr for none): ");
chop($moCompileFlags = <STDIN>);
$compileFlags =  "$compileFlags $moCompileFlags";
printf "adding compiler args $compileFlags\n";


#
# add dependancy(s) to Makefile
#
# save off old as Makefileo
my     @args =( "cp", "$MakefileIn", "$MakefileO" );
system(@args ); # || die "Failed to @args: $!";

open(MAKEIN,    "$MakefileIn")    || die "MAKIN: $!";
open(MAKEOUT, "> $MakefileOut")   || die "MAKEOUT: $!";

# add object files and CXX line
my $newCXXLine = sprintf("%s_CXXFLAGS = %s\n", $sourceFileName, $compileFlags ) ;
while(<MAKEIN>){    
  #next if /^#/; # skip comments
  if($sourceFileNameHelper =~ /.+/){  # if there is a helper file
        s/^HANDLER_OBJS =/HANDLER_OBJS = $sourceFileName.o $sourceFileNameHelper.o /o;}
  else{ s/^HANDLER_OBJS =/HANDLER_OBJS = $sourceFileName.o /o; }

  # add CXX line
  s/Handlers -target-/Handlers -target-\n$newCXXLine/o; 
  print MAKEOUT;
}


close(MAKEIN);
close(MAKEOUT);
# end Makefile section



#
#  add new(class) to all_handlers.h pushback line
#  add include to  all_handlers.h includes
#
# save off old as all_handlers.ho
rename("$allFileOut", "$allFileO")|| die "rename $allFileOut: $!";

open(ALLHIN,    "$allFileIn")     || die "ALLHIN: $!";
open(ALLHOUT, "> $allFileOut")    || die "ALLHOUT: $!";
my $newLine;
my $target;    # line to be added to
while(<ALLHIN>){
  s/^\#include "handler.h"\n/\#include "handler.h"\n\#include "$sourceFileName.h"\n/o;
  if($linkHandler == 1){
    $newLine = sprintf("BEGIN HANDLERS (link handlers only)\n  if(strcmp(conf.getValue(\"%s_ON\"       ), \"0\")) linkParser->addLinkHandler( new %s);\n",  $className, $className );
    s/BEGIN HANDLERS \(link handlers only\)\n/$newLine/o;
  }
  else{
    $target = "BEGIN HANDLERS\n";
    $newLine = sprintf("%s  if(strcmp(conf.getValue(\"%s_ON\"       ), \"0\"))  handlers.push_back( new %s());\n", $target,  $className, $className );
    s/$target/$newLine/o;
  }
  s/conf.init\(/conf.init\(\"$WEBBASEROOT\/inputs\/webbase.conf\"/o; #path to config file
  print ALLHOUT; # dump the results of all substitutions and untouched lines to out file
}

close( ALLHOUT );
close( ALLHIN  );

# end  all_handlers.h section


#
#  add path to webbase.conf 
#  add switch
#
# save off old as webbase.confo
rename("$configFileOut", "$configFileO")|| die "rename $configFileOut: $!";

open(CONFIGHIN,    "$configFileIn")     || die "CONFIGHIN: $!";
open(CONFIGHOUT, "> $configFileOut")    || die "CONFIGHOUT: $!";

while(<CONFIGHIN>){
   print CONFIGHOUT; 
}
print  CONFIGHOUT "\n#\n#\n"; 
$newLine = sprintf("%s_SCRATCH=%s\n",  $className, $path );
print  CONFIGHOUT "$newLine"; 
$newLine = sprintf("%s_ON=1\n",  $className );
print  CONFIGHOUT "$newLine"; 
close( CONFIGHOUT );

# end  webbase.conf section



printf("Ready to make?(Y/N): ");
chop($_ = <STDIN>);


if(/Y|YES/i){
  my $commandLine = "cd $WEBBASEROOT;gmake clean";
  print "Issuing $commandLine\n";
  system($commandLine);
  $commandLine = "cd $WEBBASEROOT;gmake";
  print "Issuing $commandLine\n";
  system($commandLine);
}

print "To run RunHandlers ( after gmake ) over the net: perl $WEBBASEROOT/scripts/getpages.pl\n";

exit;  
