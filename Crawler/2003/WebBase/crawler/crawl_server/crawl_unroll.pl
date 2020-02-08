#!/usr/bin/perl -w
#
# crawl_unroll.pl - undo the partial crawls for a crashed crawler
# Wang Lam <wlam@cs.stanford.edu>
# Feb 2003
#
# Usage: crawl_unroll.pl crawler-id
# This operation is repeatable, but should only need to be run once for
# each crawler that crashed.  The crashed crawler's partial crawls are
# deleted, and the sites returned to an uncrawled state.
#
#

use DBI;
use POSIX;

## command-line parameters

die "Usage: crawl_unroll.pl crawler-id {delete|preserve}\n" unless defined $ARGV[0];
$crawlerid = $ARGV[0];			# id of crawler that crashed
die "crawler-id must be numeric\n" unless $crawlerid =~ /^\d+$/;

$command = $ARGV[1];
if (defined($command)) {
	# Find nearest match for $command
	$command = 'delete' if length($command) and
		substr('delete',0,length($command)) eq $command;
	$command = 'preserve' if length($command) and
		substr('preserve',0,length($command)) eq $command;
} else {
	$command = '';
}
if ($command ne 'delete' and $command ne 'preserve') {
	die "Usage: crawl_unroll.pl crawler-id {delete|preserve}\n";
}

## setup

$DEBUG_DB = 0;			  	# print db-related debug msgs

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

## verify dead crawler, then clear out each partially-crawled site

print "Verifying crawler is not running...";
if ( `./crawl_ctl.pl status $crawlerid | grep 'Cannot connect to crawler' ` ) {
	die "Crawler $crawlerid may still be running!\n";
}

&connect();
while ((@answer = 
		&query("SELECT hid FROM crawling WHERE id = $crawlerid"))) {
	$hid = $answer[0];
	print "Found partial crawl site $hid\n" if $DEBUG_DB;

	# Get the canonical hostname
	@answer = &query("SELECT hostname FROM Hosts WHERE hid = $hid");
	$hostname = $answer[0];
	die "Could not determine hostname of site $hid: $!\n" unless $hostname;

	# Delete the site for that hostname (or not)
	if ($command eq 'delete') {
		print "Deleting partial crawl of $hostname\n";
		&deltree($hostname);
	} else {
		print "Not deleting partial crawl of $hostname\n";
	}

	# Move hid from crawling to uncrawled
	print "Restoring site into uncrawled table for future crawl\n";

	$DB_CONN->do("DELETE FROM crawling WHERE hid = $hid") ||
		die "Can't remove host $hid from table crawled: $!\n";
	$DB_CONN->do("INSERT INTO uncrawled (hid,ipaddr) VALUES \
                        ($hid, (SELECT ipaddr FROM ipaddrs
				WHERE hid = $hid ORDER BY ipaddr LIMIT 1))") ||
		die "Can't restore host $hid to table uncrawled: $!\n";

	$DB_CONN->commit() || die "Can't commit transaction: $!\n";
}

print "Incomplete Web sites for crawler $crawlerid are reset for crawl.\n";
$DB_CONN->disconnect();

# rm -r $hostname
# Reimplements feeders/repository/PathGenerator logic.  :(
#
sub deltree {
	my ($hostname) = @_;
	my (@domain) = split(/\./, $hostname);
	my $subdir = join('/',reverse @domain);
	# print "$hostname -> $subdir\n";
	my $dir = "$REPDIR[$crawlerid]/$subdir";
	print "/bin/rm -r $dir";
	system('/bin/rm', '-r', $dir);
}

