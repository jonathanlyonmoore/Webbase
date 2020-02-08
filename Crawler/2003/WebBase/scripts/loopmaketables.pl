#!/usr/bin/perl -w
#
# loopmaketables.pl - Iterate webcat|maketable over the Web repository
# Wang Lam <wlam@cs.stanford.edu>
#
# Modified <wlam@cs.stanford.edu> January 2000:
# New usage: run this script with one argument, --configfile=conffile
# where conffile is the name of the WebBase configuration file.
#
# (Old usage: Edit the variables for ##ITERATIONS and ##DEPENDENCIES
# sections to suit your current run, then invoke this script.)
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

## ITERATIONS

$ITERATION_LENGTH = &ConfLoader::getValue('ITER_LENGTH');
$ITERATION_COUNT = &ConfLoader::getValue('ITER_COUNT');
die "ITER_LENGTH not defined in $CONFIGFILE\n" unless $ITERATION_LENGTH;
die "ITER_COUNT not defined in $CONFIGFILE\n" unless $ITERATION_COUNT;

# The old way
# $ITERATION_LENGTH = 5000001;
# $ITERATION_LENGTH = 10000;
# $ITERATION_COUNT = 9;
# $ITERATION_COUNT = 1;

$CURRENT_OFFSET = 0;
$LAST_OFFSET = 0;

## DEPENDENCIES

$WEBCAT = &ConfLoader::getValue('WEBCAT');
die "WEBCAT not defined in $CONFIGFILE\n" unless $WEBCAT;
$WEBCAT .= " -l -c $ITERATION_LENGTH -s ";

$WEBCAT_REPOSITORY = &ConfLoader::getValue('REP_FILE');
die "REP_FILE not defined in $CONFIGFILE\n" unless $WEBCAT_REPOSITORY;

$MAKETABLES = "bin/maketable --all $ARGV[0] ";	# Pass on our --configfile
$MAKETABLE_APPENDFLAG = "--append";

# The old way
# $WEBCAT = "/u/cho/project/pita/webcat -l -c $ITERATION_LENGTH -s ";
# $WEBCAT_REPOSITORY = "sampleInputs/fullrep";
# # $MAKETABLES = "/dfs/pita/13/rsram/maketables sampleInputs/urlmap sampleInputs/breakpoints"; 
# $MAKETABLES = "bin/maketable --all --umap=sampleInputs/urlmap --bp=sampleInputs/breakpoints --lmap=$ENV{'LINKSDIR'} --prefix=$ENV{'OFFSETS_NAME_PREFIX'} "; 
# $MAKETABLE_APPENDFLAG = "--append";
 
$URLS_DIR = &ConfLoader::getValue('URLS_DIR');
$GZ_URLS_DIR = &ConfLoader::getValue('GZ_URLS_DIR');
die "URLS_DIR not defined in $CONFIGFILE\n" unless $URLS_DIR;
die "GZ_URLS_DIR not defined in $CONFIGFILE\n" unless $GZ_URLS_DIR;

$SORT = "/bin/sort -u -T $URLS_DIR";
$MERGE = "/bin/sort -u -m -T $URLS_DIR";

$COMPRESS = "/bin/gzip -c";
$VERIFY_COMPRESS = "/bin/gzip --test";
$UNCOMPRESS = "/bin/gunzip -c";

$REPLACE = "/bin/mv -f";

foreach $i ( "00" .. "63" ) {
	$UNCOMP_FILENAMES[$i] = "$URLS_DIR/urlsFile$i";
		# Filename for uncompressed data
	$INTER_FILENAMES[$i] = "$URLS_DIR/urlsFileSorted${i}";
		# Filename for intermediate (sorted, uniq'd) data
	$ACCUM_FILENAMES[$i] = "$GZ_URLS_DIR/urlsFile${i}.gz";
		# Filename for gzipped accumulated data
}

# The old way
# foreach $i ( "00" .. "63" ) {
# 	$UNCOMP_FILENAMES[$i] = "/dfs/chub/0/wlam/webbase-index/tmp/uncompressed-urls/urlsFile$i";
# 		# Filename for uncompressed data
# 	$INTER_FILENAMES[$i] = "$ENV{'URL_LIST_SCRATCH_DIR'}/urlsFileSorted${i}";
# 		# Filename for intermediate (sorted, uniq'd) data
# 	$ACCUM_FILENAMES[$i] = "$ENV{'URL_LIST_DIR'}/urlsFile${i}.gz";
# 		# Filename for gzipped accumulated data
# }

## PROGRAM

# Initialize the files that will accumulate the data
open(EMPTY,"cat /dev/null | $COMPRESS - |") ||
	die "Can't create gzip files: $!\n";
@empty = <EMPTY>;
close(EMPTY);
for $i ( 0 .. 63 ) {
	open(INIT,">$ACCUM_FILENAMES[$i]") ||
		die "Can't create $ACCUM_FILENAMES[$i]: $!\n";
	print INIT @empty;
	close(INIT);
}

# Do the loop
for $iteration ( 1 .. $ITERATION_COUNT ) {

	# Ensure maketables does not overwrite its previous iteration!
	$MAKETABLESCMD = $MAKETABLES;
	$MAKETABLESCMD .= " $MAKETABLE_APPENDFLAG" if ($iteration > 1);

	# Generate the uncompressed data
	$command = "$WEBCAT $CURRENT_OFFSET $WEBCAT_REPOSITORY | $MAKETABLESCMD"; 
	open(INPUT, "$command |") ||
		die "Can't run $command: $!\n";
	print STDERR "Running iteration $iteration...\n";
	print STDERR "$command\n";
	while(<INPUT>) {
		# Toss most status messages on stdout
		(print STDERR $_), next if /^Opening/;
		(print STDERR $_), next if /^Appending/;
		(print STDERR $_), next if /^Finished/;

		# Keep the last offset for the next run
		chomp $_;
		$CURRENT_OFFSET = $_;
		warn "maketable didn't return numeric final offset: $_!\n"
			if (!($CURRENT_OFFSET =~ /^\d+$/));
	}
	if ($CURRENT_OFFSET eq $LAST_OFFSET) {
		print STDERR "End of repository reached in last iteration.\n";
		last;
	}
	print STDERR "Uncompressed data generated.\n";

	foreach $j ( 0 .. 63 ) # maketables generates 64 buckets
	{
		# Compress the newly generated data
		print STDERR "File $j: sort/reduce...";
		$command = "$SORT $UNCOMP_FILENAMES[$j] > $INTER_FILENAMES[$j]";
#		print STDERR "\n$command\n";
		$retcode = system($command);
		if ($retcode) {
			print STDERR "\nError: Nonzero return code $retcode!\n";
		}

		# Merge data with accumulated store
		# It so happens that the accumulated store has a bunch of
		# empty-content gzip files, since the user put them there,
		# right?  :)
		print STDERR "merge...";
		$command = "$UNCOMPRESS $ACCUM_FILENAMES[$j] | $MERGE - $INTER_FILENAMES[$j] | $COMPRESS - > $ACCUM_FILENAMES[$j].new";
#		print STDERR "\n$command\n";
		$retcode = system($command);
		if ($retcode) {
			print STDERR "\nError: Nonzero return code $retcode!\n";
		}
		$command = "$REPLACE $ACCUM_FILENAMES[$j].new $ACCUM_FILENAMES[$j]\n";
		$retcode = system($command);
		if ($retcode) {
			print STDERR "\nError: nonzero return code $retcode from $command!\n";
		}
		
		# Verify compressed accumulated data
		print STDERR "verify...";
		$command = "$VERIFY_COMPRESS $ACCUM_FILENAMES[$j]";
#		print STDERR "\n$command\n";
		$retcode = system($command);
		if ($retcode) {
			print STDERR "Error: $VERIFY_COMPRESS returned $retcode!\n" ;
		}

		print STDERR "done.\n";

		# Check for quit
		die "Quitting!  Found QUIT file.\n" if ( -f "QUIT" );
	}

	# Check for terminate
	die "Ending at iteration $iteration.  Found TERM file.\n" if ( -f "TERM" );
}

print STDERR "Iterations finished.\n";
exit 0;
