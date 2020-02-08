#!/usr/bin/perl -w
#
# crawl_roll.pl - find the finished partial crawls for a crawler
# Wang Lam <wlam@cs.stanford.edu>
# Jun 2003
#
# Usage: crawl_roll.pl crawler-id
# This operation is repeatable, but should only need to be run once for
# each crawler after its termination.  The crawler's partial crawls are
# checked for completion in the crawler's logs, and if the site is
# finished, it is submitted to an crawled state.
#
# If crawl_unroll.pl is also used, crawl_roll.pl should always go first.
# crawl_unroll.pl must not run until after crawl_roll.pl is finished.
#
# Known issues (i.e., this is a hack):
# * does not mutex-lock to block crawl_unroll
#

use DBI;
use POSIX;

## command-line parameters

die "Usage: crawl_roll.pl crawler-id\n" unless defined $ARGV[0];
$crawlerid = $ARGV[0];			# id of crawler that crashed
die "crawler-id must be numeric\n" unless $crawlerid =~ /^\d+$/;

## setup

$DEBUG_DB = 0;			  	# print db-related debug msgs
$DEBUG_LOG = 0;				# print log-scanning-related msgs
$DEBUG_DATE = 0;			# print cmp_date debug msgs

$EGREP = "nice -n 1 /bin/egrep";
$CAT = "/bin/cat";
$HEAD = "/usr/bin/head";
$TAIL = "/usr/bin/tail";
printf "Pause crawler?\n";
chomp($_ = <STDIN>);
if( /y/ ){
    `perl crawl_ctl.pl pause $crawlerid`;
    printf "paused\n";
    $paused = 1;
}
else { $paused = 0;}

## more globals

# Read crawl_config; fill %%CONFIG_VAR. (taken from crawl_ctl.pl)
$config_file = "crawl_config";
open(CONFIG,"<$config_file") || die "Cannot open $config_file: $!\n";
$section = "";
while(<CONFIG>) {
	if (/^\[(.*)]/) {
		$section = $1;
	} elsif (/^([^#=][^=]*)=(.*)$/) {
		$CONFIG_VAR{$section}{$1} = $2;
	}
}
close(CONFIG);

# Now we must pick up the repository directories too.
$CONFIG_DIR[$crawlerid] = "$CONFIG_VAR{'crawlserver'}{'ConfigFile'}.$crawlerid";
$REPDIR[$crawlerid] = `/bin/cat $CONFIG_DIR[$crawlerid]`;
chomp $REPDIR[$crawlerid];

$DB = $CONFIG_VAR{'siteserver'}{'db'};	# name of database to use
$DB_USER = 				# database username
	$CONFIG_VAR{'siteserver'}{'dbuser'};
$DB_PASSWD = 				# database username password
	$CONFIG_VAR{'siteserver'}{'dbpasswd'};
$DB_CONN = undef;			# handle to database

## main loop

## dbi subroutines (from crawl_buddy.pl) (these subs should be factored :( )

# A quick one-line 'ensure db is connected'
sub connect {
	if (defined($DB_CONN) and (!$DB_CONN->{Active} or !$DB_CONN->ping())) {
		$DB_CONN->disconnect();
		$DB_CONN = undef;
	}
	if (!defined($DB_CONN)) {
		# undef, undef -> username, passwd
		$DB_CONN = DBI->connect("DBI:Pg:dbname=$DB",$DB_USER,$DB_PASSWD,
			{ AutoCommit => 0 });
	}
	return $DB_CONN;
}

# A quick one-line query for a one-line answer
# Returns list with answer, or () (not undef!) if not possible.
sub query {
	my ($sql) = @_;
	my $sth = $DB_CONN->prepare($sql) ||
		return undef;
	$sth->execute() || return undef;
	return () unless $sth->rows();

	return $sth->fetchrow_array();
}

## crawl log date regexps (also see inside cmp_date)

# crawl_buddy log pattern:
# [Thu Apr 10 19:35:31 2003] Requesting crawl bonsai.mozilla.org
$BUDDY_REGEXP = '\[\w\w\w \w\w\w [ 123]\w \w\w:\w\w:\w\w \w\w\w\w\]';

# crawl_server log pattern
# [19:21:24 04/10] LoadRequest: www.infotech.monash.edu.au (id: 3 active_count: 3)
$SERVER_REGEXP = '\[\w\w:\w\w:\w\w \w\w\/\w\w\]';

## debug section only - do not use

if ( 0 ) {
	$| = 1;
	print "addsite(0,www.infotech.monash.edu.au)\n";
	&addsite(0,'www.infotech.monash.edu.au');

	print "addsite(0,doctor.mozilla.org)\n";
	&addsite(0,'doctor.mozilla.org');

	$TABLED = 2;
	goto beginscan;
}


## verify dead crawler (optional)
# Does not work!
print "Verifying crawler is not running...";
if ( `./crawl_ctl.pl status $crawlerid | grep 'Cannot connect to crawler' ` ) {
	die "Crawler $crawlerid may still be running!\n";
}

## register all the sites to examine

$TABLED = 0;
%HID = ();
{
print "Getting sites from database...\n";
&connect();
$handle = $DB_CONN->prepare("SELECT hid FROM crawling WHERE id = $crawlerid") or
	die "Cannot create query: $!\n";
$handle->execute() or die "Cannot run query: $!\n";
while (($hid) = $handle->fetchrow_array()) {

	print "Found partial crawl site $hid\n" if $DEBUG_DB;
	++$TABLED;

	# Get the canonical hostname
	($hostname) = &query("SELECT hostname FROM Hosts WHERE hid = $hid");
	die "Could not determine hostname of site $hid: $!\n" unless $hostname;

	# Register the site as one to check
	if (&addsite($crawlerid,$hostname)) {
		print "$hostname\n";
		$HID{$hostname} = $hid;
	} else {
		print "$hostname skipped: not found in crawl_buddy logs\n";
	}
}
}

$c = ""; $num = 0;
#Sites 11
$ans = `./crawl_ctl.pl status $crawlerid | grep Sites` ;
($c ,$num) = split(/ /, $ans ); # parse line into tokens
#printf "%2s %3i |", $c, $num; 

if( $num == $TABLED){
    if( $paused ) {
	`perl crawl_ctl.pl resume $crawlerid`;
	printf "resumed\n" ;
    }
    printf "Correct total of $num, no cleanup needed\n";
    exit 0;
}
if (scalar keys %HID == 0) {
    print "No sites found to verify.\n";
    exit 0;
}
else { print "$TABLED sites found to verify > $num reported via status\n"; }


## skim the tops of crawl_server logs until we reach the right date

beginscan:

print "Finding crawl_server logs...\n";
@lognums = &findlognums(
	(sort { &cmp_date_long($a,$b) } values %BEGINDATE)[0]	# min req date
);

## scan crawl_server logs

$logfiles = join ' ',
	map { "log/crawl_server.$crawlerid.".sprintf("%03d",$_) } @lognums;
print "Scan log files: ".join(' ',$logfiles)."\n" if $DEBUG_LOG;

$STARTED = 0;
$bytes = 0;
open(LOG, "$CAT $logfiles |") || die "Can't open logs: $!\n";
$| = 1;
print "Scanning logs...\n";
while(<LOG>) {
	#print "FindLine: $_" if $DEBUG_LOG;
	print "."
		if (int($bytes/(1*1024*1024)) != 
			int(($bytes+length)/(1*1024*1024)));
	print "\nScanned ".int(($bytes+length)/(1024*1024))." megabytes\n"
		if (int($bytes/(50*1024*1024)) != 
			int(($bytes+length)/(50*1024*1024)));
	$bytes += length;

	# crawl request
	if (m{^$SERVER_REGEXP LoadRequest: }) {
		my($date,$site) = m{^($SERVER_REGEXP) LoadRequest: ([^ ]+) };
		if (defined($BEGINDATE{$site}) and 
			&cmp_date_short($BEGINDATE{$site},$date) < 1 ) {
			warn "Crawl requested for $site again on $date\n"
				if defined($CRAWLSTARTED{$site}) &&
					$CRAWLSTARTED{$site};
			$CRAWLSTARTED{$site} = 1;
			$FETCHED{$site} = 0;
			$STATUS{$site} = 'Incomplete';
			++$STARTED;
			print "Crawl request: $_" if $DEBUG_LOG;
		} # elsif (defined($BEGINDATE{$site})) {
		# 	warn "Skipped request: $BEGINDATE{$site} $date $site\n";
		# }
	}

	# page fetched
	if ( m{^$SERVER_REGEXP Done: } ) {	# ...[:port]/[path] (.*)$
		my($site) = m{^$SERVER_REGEXP Done: ([^ :/]+)};
		if (defined($CRAWLSTARTED{$site}) and
			$CRAWLSTARTED{$site} == 1) {
			++$FETCHED{$site};
		}
	}

	# crawl finished
	if ( m{^$SERVER_REGEXP SiteDone: } ) {print "Line: $_" if $DEBUG_LOG;
		my($site,$status) = m{^$SERVER_REGEXP SiteDone: ([^ ]+) \(.*status: (.*)\)$};
		if (defined($CRAWLSTARTED{$site}) and
			$CRAWLSTARTED{$site} == 1) {
			# print "Line: $_" if $DEBUG_LOG;
			# print "Crawl complete: $_" if $done; # if verbose
			print "Finished: $site $FETCHED{$site} $status\n" if $DEBUG_LOG;
			push(@DONE,$site);
			$STATUS{$site} = $status;
		}
	}

}
close(LOG);
print "\nScanning finished.\n";

## act on finished site crawls

$COMMITTED = 0;
foreach $site (@DONE) {
	print "Moving site $site into crawled table...";

	if (0) { print "skipped.\n"; next; }; # debug section only - do not use

	$DB_CONN->do("DELETE FROM crawling WHERE hid = $HID{$site}") ||
		die "Can't remove host $HID{$site} from table crawled: $!\n";
	$DB_CONN->do("INSERT INTO crawled (hid,id,fetched,status) VALUES \
                ($HID{$site}, $crawlerid, $FETCHED{$site}, '$STATUS{$site}')")
		|| die "Can't add host $HID{$site} to table crawled: $!\n";

	if (! $DB_CONN->commit()) {
		warn "Can't commit transaction: $!\n";
	} else {
		print "done.\n";
		++$COMMITTED;
	}
}

## assess the damage

$SEARCHED = scalar(keys %HID);
$FINISHED = scalar(@DONE);

print <<EOF;
Statistics for crawler $crawlerid:
   $TABLED site(s) in database as crawling
   $SEARCHED of $TABLED site(s) found in crawl_buddy logs
   $STARTED of $SEARCHED site(s) found in crawl_server logs
   $FINISHED of $STARTED site(s) finished crawling
   $COMMITTED of $FINISHED site(s) have status updated in database

EOF

$DB_CONN->disconnect();
`perl crawl_ctl.pl resume $crawlerid`;
exit 0;

############################################################################
## functions

# Register a hostname to examine in logs
sub addsite {
	my ($crawlerid, $hostname) = @_;
	print "addsite(id $crawlerid, host $hostname)\n" if $DEBUG_LOG;

	# Find last crawl of site in crawl_buddy
	my $req = "$EGREP '^$BUDDY_REGEXP Requesting crawl $hostname' log/crawl_buddy.log.$crawlerid | $TAIL -1";
	print "Command: $req\n" if $DEBUG_LOG;
	$req = `$req`;
	if (!defined($req)) {
		warn "Cannot find crawl_buddy request for $hostname\n";
		return undef;
	}
	print "Found last crawl: $req" if $DEBUG_LOG;
	$req =~ /^($BUDDY_REGEXP)/;
	my $reqdate = $1;
	print "Found last crawl date: $reqdate\n" if $DEBUG_LOG;

	# Record date of last crawl 
	$BEGINDATE{$hostname} = $reqdate;
	return 1;
}

# Given a datestamp, find a small sequence of crawl_server logs that include it
sub findlognums {
	my($reqdate) = @_;

	# Get top lines of each crawl_server log, and extract its datestamp
	my @start = glob "log/crawl_server.$crawlerid.[0-9][0-9][0-9]";
	@start = map { `$HEAD -1 $_` } @start;
	my $lognum = 0;
	map { chomp; s/^($SERVER_REGEXP) .*$/$1/; } @start;

	# Find crawl_server log file with nearest timestamp
	while ($lognum+1 < scalar(@start)) {
		print "Log $lognum+1: $start[$lognum+1]\n" if $DEBUG_LOG;
		if (&cmp_date_short($reqdate,$start[$lognum+1]) > 0) {
			++$lognum;
		} else {
			last;
		}
	};
	#if( $lognum > 0) { --$lognum; } # kludge sometimes needed

	print "Start log: $lognum\n" if $DEBUG_LOG;

	return ($lognum) if ($lognum == scalar(@start) - 1);
	return ($lognum .. scalar(@start)-1);
}

# This is where Date::Manip would do, but it's not on the machine
# where I'm writing this and I don't get to install things on it,
# so I'll take that as a hint that I shouldn't start adding CPAN 
# dependencies on other people either.
#
# [22:06:20 06/19]
# Compare datestamps a and b
# Returns -1 if a < b, 0 if a = b, 1 if a > b
sub cmp_date_short {
	%MONTH = ('Jan' => 1,
		'Feb' => 2,
		'Mar' => 3,
		'Apr' => 4,
		'May' => 5,
		'Jun' => 6,
		'Jul' => 7,
		'Aug' => 8,
		'Sep' => 9,
		'Oct' => 10,
		'Nov' => 11,
		'Dec' => 12);
	my ($a, $b) = @_;
	print "compare >$a< to >$b< (short)\n" if $DEBUG_DATE;
	# [Thu Apr 10 19:35:31 2003]
	$a =~ '^\[?\w\w\w (\w\w\w) ( \d|\d\d) (\d\d):(\d\d):(\d\d) (\d\d\d\d)\]?';
	my ($ayear, $amonth, $aday, $ahour, $amin, $asec) = ($6, $1, $2, $3, $4, $5);
	$amonth = $MONTH{$amonth};

	#this is different for short
	#[22:06:20 06/19]
	$b =~ '^\[?(\d\d):(\d\d):(\d\d) (\d\d)/(\d\d)\]?';
	my ($byear, $bmonth, $bday, $bhour, $bmin, $bsec) = ($ayear, $4, $5, $1, $2, $3);
	
	print "ayear >$ayear< byear >$byear<\n" if $DEBUG_DATE;
	return -1 if $ayear < $byear;
	return 1 if $ayear > $byear;
	# return &cmp_month($amonth,$bmonth) if &cmp_month($amonth,$bmonth) != 0;
	print "amonth >$amonth< bmonth >$bmonth<\n" if $DEBUG_DATE;
	return -1 if $amonth < $bmonth;
	return 1 if $amonth > $bmonth;
	print "aday >$aday< bday >$bday<\n" if $DEBUG_DATE;
	return -1 if $aday < $bday;
	return 1 if $aday > $bday;

	print "ahour >$ahour< bhour >$bhour<\n" if $DEBUG_DATE;
	return -1 if $ahour < $bhour;
	return 1 if $ahour > $bhour;
	print "amin >$amin< bmin >$bmin<\n" if $DEBUG_DATE;
	return -1 if $amin < $bmin;
	return 1 if $amin > $bmin;
	print "asec >$asec< bsec >$bsec<\n" if $DEBUG_DATE;
	return -1 if $asec < $bsec;
	return 1 if $asec > $bsec;

	return 0;
}

# This is where Date::Manip would do, but it's not on the machine
# where I'm writing this and I don't get to install things on it,
# so I'll take that as a hint that I shouldn't start adding CPAN 
# dependencies on other people either.
#
# [Thu Apr 10 19:35:31 2003]
# Compare datestamps a and b
# Returns -1 if a < b, 0 if a = b, 1 if a > b
sub cmp_date_long {
	%MONTH = ('Jan' => 1,
		'Feb' => 2,
		'Mar' => 3,
		'Apr' => 4,
		'May' => 5,
		'Jun' => 6,
		'Jul' => 7,
		'Aug' => 8,
		'Sep' => 9,
		'Oct' => 10,
		'Nov' => 11,
		'Dec' => 12);
	my ($a, $b) = @_;
	print "compare >$a< to >$b< (long)\n" if $DEBUG_DATE;
	# [Thu Apr 10 19:35:31 2003]
	$a =~ '^\[?\w\w\w (\w\w\w) ( \d|\d\d) (\d\d):(\d\d):(\d\d) (\d\d\d\d)\]?';
	my ($ayear, $amonth, $aday, $ahour, $amin, $asec) = ($6, $1, $2, $3, $4, $5);
	$amonth = $MONTH{$amonth};

	#this is the different part for long
        #[Mon Jun 23 08:51:56 2003]
	$b =~ '^\[?\w\w\w (\w\w\w) (\d\d?) (\d\d):(\d\d):(\d\d) (\d\d\d\d)\]?';
	my ($byear, $bmonth, $bday, $bhour, $bmin, $bsec) = ($6, $1, $2, $3, $4, $5);
	

	$bmonth = $MONTH{$bmonth};
	
	print "ayear >$ayear< byear >$byear<\n" if $DEBUG_DATE;
	return -1 if $ayear < $byear;
	return 1 if $ayear > $byear;
	# return &cmp_month($amonth,$bmonth) if &cmp_month($amonth,$bmonth) != 0;
	print "amonth >$amonth< bmonth >$bmonth<\n" if $DEBUG_DATE;
	return -1 if $amonth < $bmonth;
	return 1 if $amonth > $bmonth;
	print "aday >$aday< bday >$bday<\n" if $DEBUG_DATE;
	return -1 if $aday < $bday;
	return 1 if $aday > $bday;

	print "ahour >$ahour< bhour >$bhour<\n" if $DEBUG_DATE;
	return -1 if $ahour < $bhour;
	return 1 if $ahour > $bhour;
	print "amin >$amin< bmin >$bmin<\n" if $DEBUG_DATE;
	return -1 if $amin < $bmin;
	return 1 if $amin > $bmin;
	print "asec >$asec< bsec >$bsec<\n" if $DEBUG_DATE;
	return -1 if $asec < $bsec;
	return 1 if $asec > $bsec;

	return 0;
}

