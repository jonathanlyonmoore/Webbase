#!/usr/bin/perl -w
#
# move all crawling for a crawler to crawled
# Gary Wesley 6/03
#
# Usage: move2crawled.pl crawlerid
use DBI;
use POSIX;

## command-line parameters

die "Usage:  move2crawled.pl crawlerid\n" unless defined $ARGV[0];
$crawlerid = $ARGV[0];			# id of crawler that crashed
#die "crawler-id must be numeric\n" unless $crawlerid =~ /^[\d+]$/;

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
	print "Found partial(?) crawl site $hid\n" if $DEBUG_DB;

	# Get the canonical hostname
	@answer = &query("SELECT hostname FROM Hosts WHERE hid = $hid");
	$hostname = $answer[0];
	die "Could not determine hostname of site $hid: $!\n" unless $hostname;

	# (Delete the site for that hostname)
	print "Saving partial(?) crawl of $hostname\n";

	# Move hid from crawling to uncrawled
	#print "Moving site into crawled table\n";
	my $cstatus = 'Unknown';
	$DB_CONN->do("DELETE FROM crawling WHERE hid = $hid") ||
		die "Can't remove host $hid from table crawled: $!\n";
	$DB_CONN->do("INSERT INTO crawled (hid,id, status) VALUES \
                        ($hid, $crawlerid, 'Unknown')") ||
		die "Can't host $hid to table crawled: $!\n";

	$DB_CONN->commit() || die "Can't commit transaction: $!\n";
}

print "Incomplete Web sites for crawler $crawlerid are reset for crawl.\n";
$DB_CONN->disconnect();

