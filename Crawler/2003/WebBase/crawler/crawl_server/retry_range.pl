#!/usr/bin/perl -w
#
# retry_range.pl - retry crawls for sites x-y
# Gary Wesley from Wang Lam <wlam@cs.stanford.edu>
# April 2003
#
# Usage: retry_range.pl begin end
# The too_many_errors crawls are
# deleted, and the sites returned to an uncrawled state.
#
#

use DBI;
use POSIX;

## command-line parameters

die "Usage:  retry_range.pl begin end \n" unless defined $ARGV[0];
$bthreshold = $ARGV[0];			# begin of range
$ethreshold = $ARGV[1];			# end of range

## setup

$DEBUG_DB = 0;			  	# print db-related debug msgs

## more globals

$DB = "Med-08-03";#$CONFIG_VAR{'siteserver'}{'db'};	# name of database to use
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


&connect();
while ((@answer = 
		&query("select hid  from crawled where  hid > $bthreshold and hid < $ethreshold "))) {
	$hid = $answer[0];
	print "Found partial crawl site $hid\n" if $DEBUG_DB;

	# Get the canonical hostname
	@answer = &query("SELECT hostname FROM Hosts WHERE hid = $hid");
	$hostname = $answer[0];
	die "Could not determine hostname of site $hid: $!\n" unless $hostname;

	# (Delete the site for that hostname)
	print "Retry crawl of $hostname\n";

	# Move hid from crawled to uncrawled
	print "Restoring site into crawled table for future crawl\n";

	$DB_CONN->do("DELETE FROM crawled WHERE hid = $hid") ||
		die "Can't remove host $hid from table crawled: $!\n";
	$DB_CONN->do("INSERT INTO uncrawled (hid,ipaddr) VALUES \
                        ($hid, (SELECT ipaddr FROM ipaddrs
				WHERE hid = $hid ORDER BY ipaddr LIMIT 1))") ||
		die "Can't restore host $hid to table uncrawled: $!\n";

	$DB_CONN->commit() || die "Can't commit transaction: $!\n";
}

print "Failed Web sites  hid > $bthreshold and hid < $ethreshold are reset for crawl.\n";
$DB_CONN->disconnect();

