#!/usr/bin/perl -w
#
# retry_one.pl - retry crawl for a failed site
# Gary Wesley from Wang Lam <wlam@cs.stanford.edu>
# April 2003
#
# Usage: retry_one.pl id
# The crawl is
# deleted, and the site returned to an uncrawled state.
#
#

use DBI;
use POSIX;
# Read crawler_config; fill %%CONFIG_VAR.
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
# For general use, this sub should pick up the config dirs too. :P


## command-line parameters

die "Usage: retry_one.pl id\n" unless defined $ARGV[0];
$site = $ARGV[0];			# hid  that crashed

## setup

$DEBUG_DB = 0;			  	# print db-related debug msgs

## more globals

$DB = $CONFIG_VAR{'siteserver'}{'db'};	# name of database to use
print "database $DB \n";

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
		&query("select hid  from crawled where hid = $site"))) {
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

print "Failed Web sites id = $site $hostname are reset for crawl.\n";
$DB_CONN->disconnect();

