#!/usr/bin/perl -w
#
# partition-repfile.pl - Partition repository file by partition
# Wang Lam <wlam@cs.stanford.edu>
# 30 Mar 2001
# 
# Run this script with one argument, --configfile=conffile
# where conffile is the name of the WebBase configuration file.
#
#

## CONFIGURATION FILE

die "Usage: --configfile=configuration_filename\n"
	unless $ARGV[0] =~ /^--configfile=/;

# Find the path where the WebBase code tree begins.
use FindBin;
$_ = $FindBin::Bin;  # happens to end without a slash
s#/([^/]+)$#/# ;
$WEBBASEROOT = $_;   # happens to end in a slash

# Find confloader.pl, and the configuration file
require "$WEBBASEROOT/utils/confloader.pl";
($CONFIGFILE) = $ARGV[0] =~ /^--configfile=(.*)$/;
die "Can't load configuration file $CONFIGFILE\n" unless
	&ConfLoader::loadValues($CONFIGFILE);

## PARAMETERS

$REPFILE = &ConfLoader::getValue('REP_FILE');
$PARTITIONDIR = &ConfLoader::getValue('REP_DIR');
die "REP_FILE not defined in $CONFIGFILE\n" unless $REPFILE;
die "REP_DIR not defined in $CONFIGFILE\n" unless $PARTITIONDIR;

open(INPUT,"< $REPFILE") || die "Can't open repository file $REPFILE: $!\n";
( -d $PARTITIONDIR ) || mkdir($PARTITIONDIR,0777) ||
   die "Can't mkdir $PARTITIONDIR: $!\n";

## INPUT

%PARTITIONS = ();

# Stat every file in REPFILE, and bucket by its storage device.
# This may give different NFS mounts different buckets, rather
# than grouping them all together as a "network" device.
while (<INPUT>) {
   chomp;
   my($file) = $_;
   my($devnum) = stat($file);
   if (!defined($devnum)) {
      warn "Cannot stat $file; ignored.\n";
      next;
   }
   $PARTITIONS{$devnum} .= "$file\n";
}

## OUTPUT

$partnum = 0;
foreach $p (sort keys %PARTITIONS) {
   ++$partnum;
   my($file) = "$PARTITIONDIR/$partnum";
   open(OUTPUT,"> $file") || die "Can't open $file for writing: $!\n";
   print OUTPUT $PARTITIONS{$p};
   close(OUTPUT);
}

# Record how many partitions we have
open(OUTPUT,"> $PARTITIONDIR/0") || 
   die "Can't open $PARTITIONDIR/0 for writing: $!\n";
print OUTPUT "$partnum\n";
close(OUTPUT);

exit 0;

