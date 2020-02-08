#!/usr/bin/perl -w
#
# crawl_buddy.pl - control a crawl_server
# Wang Lam <wlam@cs.stanford.edu>
# June 2002
#
# crawl_buddy.pl replaces the siteserver + crawl_servers + starter setup
# with a database + (crawl_buddy.pl+crawl_server) setup.
#
# Each crawl_buddy.pl spawns a crawl_server, then feeds it sites
# from a database, acting as the crawl_server's "siteserver" (coordinator).
# In this setup, siteserver and starter are unneeded.  crawl_buddy.pl
# terminates crawl_server and itself automatically when all sites in the
# database are crawled.  crawl_buddy.pl automatically dies if its crawl_server
# dies.  (In this case, restart crawl_buddy.pl to continue; crawl state is
# stored in a database, not a siteserver.)
#
# Usage:
# crawl_binaries> ./crawl_buddy.pl -id [nonnegative integer] [-v[erbose]]
#
# To stop a crawl_buddy most gracefully, send it a SIGUSR1,
# then wait patiently for the currently-crawling sites to complete.
#
# 

## TODO
# -add starter here; fork crawl_server here
# -change request_crawl to real (networked) call
# -crawl_server needs to return Sites (num of crawling sites)
# register self to database?

use Getopt::Long;
use IO::Socket::INET;
use DBI;
use POSIX;

## setup

$DEBUG_CONN = 0;			# print connection-related debug msgs
$DEBUG_DB = 0;				# print db-related debug msgs

$VERBOSE = 0;				# run as foreground (instead of daemon)

# $DB = 'webbase host=oh';		# name of database to use
$DB = '';				# name of database to use
# $DB_USER = 'webbase';			# database username
$DB_USER = '';				# database username
# $DB_PASSWD = 'wbwb';			# database username password
$DB_PASSWD = '';			# database username password

$LOG_PREFIX = "log/crawl_buddy.log.";	# log error messages

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
# For general use, we should pick up the config dirs too. :P

$ID = -1;				# crawler ID
	Getopt::Long::GetOptions("id=i" => \$ID,
				 "verbose" => \$VERBOSE);
($LISTEN_PORT,$CRAWLER_PORT) =		# $0's, crawl_server's listen ports
	&getports($ID);
$DB_CONN = undef;			# handle to database
$CRAWL = 1;				# to crawl or not to crawl
$CONCURRENT_SITES = $CONFIG_VAR{'starter'}{'Threads'};
					# max concurrent sites for crawl_server
$LOG_NAME = "$LOG_PREFIX$ID";		# error log

## main loop

die "Usage: $0 -id <crawler id> [-v[erbose]]\n" unless $ID >= 0;
if (! $VERBOSE) {
	open (LOG, ">>$LOG_NAME") ||
		die "Cannot open log file $LOG_NAME: $!\n";
	select(LOG);  $| = 1;  # Quell "LOG used only once" warning
};

# Check for a corresponding crawl_server
if (&status($CRAWLER_PORT) >= 0) {
	die "crawl_server already running on $CRAWLER_PORT ".
		"or port already taken!";
};

# spawn crawl_server
# too many spurious CHLD signals: just let &status fail
$SIG{CHLD} = sub { wait; }; # die "crawl_server died; aborting.\n"; };
print "[".scalar(localtime)."] Starting crawl_server...\n" if $VERBOSE;
unless ($CRAWLER_PID = fork()) {
	open(STDOUT,">>/dev/null");
	open(STDERR,">>/dev/null");
	exec(split(/ /,"./crawl_server -id $ID -l $CRAWLER_PORT -s $LISTEN_PORT")) || die "Can't exec ./crawl_server: $!\n";
}
sleep 3;  # crawl_server needs time to set up...
die "./crawl_server failed, cannot continue.\n"
	unless &status($CRAWLER_PORT) >= 0;

# Listen
$LS = IO::Socket::INET->new( "LocalAddr" => $LISTEN_PORT,
			     "Proto"	 => "tcp",
			     "Listen"	 => 300  		);
					  # MAX_NUM_QUEUES 
die "Can't create listen socket: $!\n" unless $LS;
$LS->listen() || die "Can't listen on $LISTEN_PORT: $!\n";

# Daemonize
unless ($VERBOSE) {
	POSIX::setsid();
	chdir('/');
	close(STDOUT);
	close(STDERR);
	open STDOUT, ">&LOG";
	open STDERR, ">&LOG";
	STDOUT->autoflush(1);
	STDERR->autoflush(1);
	fork && exit;
}

$LOG_USR1 = 0; # Declare USR1 received?
$SIG{USR1} = sub { $CRAWL = 0; $LOG_USR1 = 1; };

# Starter
print "[".scalar(localtime)."] Beginning crawl of $CONCURRENT_SITES ".
	"maximum concurrent sites...\n";
print "[".scalar(localtime)."] Information for crawl_server: ".
	"Port $CRAWLER_PORT PID $CRAWLER_PID\n";
print "[".scalar(localtime)."] Information for crawl_buddy.pl: ".
	"Port $LISTEN_PORT PID $$\n";
# while (&status($CRAWLER_PORT) < $CONCURRENT_SITES) {
if (&status($CRAWLER_PORT) < $CONCURRENT_SITES) {
	last if !$CRAWL;
	my ($sitename,$ip,$aliases,$maxpages,$maxdepth,$minpause,@rooturls) =
		&getsite();
	if (!defined($sitename)) {
		print "[".scalar(localtime)."] No more sites available to crawl.\n";
		# last;  # give up trying to add more sites
	} else {
		print "[".scalar(localtime)."] Requesting crawl $sitename\n";
		&request_crawl($CRAWLER_PORT,$sitename,$ip,$aliases,
			$maxpages,$maxdepth,$minpause,@rooturls);
	}
	$LS->timeout(1);
}

# Main loop
while(1) {
	my $CS = $LS->accept();
	$LS->timeout(1);
	# next unless defined($CS);
	if (defined($CS)) {
	$CS->timeout(10);

	# Get report of finished site
	my $site = '';
	my $endstatus = '';
	my $dummyreport = 0;
	my $cmd = '';
	while (<$CS>) {
		# should, but doesn't verify the message is Type:Report
		# should, but doesn't verify that CrawlerId matches db
		print "[".scalar(localtime)."] Message: $_" if $DEBUG_CONN;
		$dummyreport=1		if /^Status: Dummy$/;
		$site = $1		if /^SiteName: (.*)$/;
		$endstatus = $1		if /^Status: (.*)$/;
		# (print "End message\n", last) unless /\w/;

		$cmd = 'die', last	if /^DIE/;
	}
	if ($cmd eq 'die') {
		# Drop what we're doing, kill crawler and die.
		close($CS);
		last;
	}
	print $CS "Status:OK\r\n\r\n";
	close($CS);
	$site && $endstatus or
		warn "Malformed incoming site-done message.\n";

	# Update status for finished site
	if (!$dummyreport and $site and $endstatus) {
		chomp $site;
		chomp $endstatus;
		print "[".scalar(localtime)."] Finished crawl $site: $endstatus\n";
		&releasesite($site, $endstatus);
	}

	} # if defined($CS)

	# Optional "Should I stop crawling?" decision-making goes here. #
	if (&status($CRAWLER_PORT) == -1) {
		print "[".scalar(localtime)."] Crawler lost, aborting.\n";
		last;
	}
	if ($LOG_USR1) {
		warn "New crawling halted: SIGUSR1 acknowledged.\n";
		$CRAWL = 0;  # redundant
		$LOG_USR1 = 0;
	}

	# Get a new site to replace finished site
	my ($sitename,$ip,$aliases,$maxpages,$maxdepth,@rooturls) = undef;
	if ($CRAWL && (&status($CRAWLER_PORT) < $CONCURRENT_SITES)) {
		($sitename,$ip,$aliases,$maxpages,$maxdepth,$minpause,
			@rooturls) = &getsite();
		if (!defined($sitename)) {
			print "[".scalar(localtime).
				"] No more sites available to crawl.\n"
		}
	}

	# Crawl new site
	if ($CRAWL && defined($sitename)) {
		print "[".scalar(localtime)."] Requesting crawl $sitename\n";
		&request_crawl($CRAWLER_PORT,$sitename,$ip,$aliases,
			$maxpages,$maxdepth,$minpause,@rooturls);
		$LS->timeout(1);
	} else {
		# Wait until all sites being crawled are finished.
		last if &status($CRAWLER_PORT) <= 0;
	}
}

# exit when done (or should this be while(1) getsite()?)
print "[".scalar(localtime)."] Crawl finished: ".
	"cleaning up database and crawl_server\n";
$DB_CONN->disconnect();
print "[".scalar(localtime)."] Kill crawler: " if $DEBUG_CONN;
my $retvalue = &kill_crawler($CRAWLER_PORT);
print "$retvalue\n" if $DEBUG_CONN;
exit 0;

## supporting procedures

# read crawl_config to determine port numbers
# returns (listen:port, crawler:port)
sub getports {
	my ($ID) = @_;
	$_ = $CONFIG_VAR{"siteserver"}{"LocalAddress"};
	# my ($localbase) = /^([^,]+)/;
	my ($localbase) = scalar((split(',',$_))[$ID]);

	$_ = $CONFIG_VAR{"crawlserver"}{"LocalAddress"};
	# my ($crawlerbase) = /^([^,]+)/;
	my ($crawlerbase) = (split(',',$_))[$ID];

	my ($machine,$portnum) = $localbase =~ /(.*\:{0,1})(\d+)$/;
	# $portnum += $ID;
	$localbase = "$machine$portnum";

	($machine,$portnum) = $crawlerbase =~ /(.*\:{0,1})(\d+)$/;
	# $portnum += $ID;
	$crawlerbase = "$machine$portnum";

	print "[".scalar(localtime)."] Port assignments: $localbase $crawlerbase\n"
		if $DEBUG_CONN;

	return ($localbase, $crawlerbase);
	# return ("localhost:".(20000 + $ID),"localhost:".(6000 + $ID));
}

# get crawler status
# in: port to connect to (e.g. "localhost:1024")
# out: number of sites being crawled at the moment (or -1 if unavail)
sub status {
	my ($PORT) = @_;
	my $S = IO::Socket::INET->new( "PeerHost" => $PORT ,
					 "Proto" => "tcp" );
	return -1 unless $S;
	my $answer = -1;
	print $S "STATUS\r\n\r\n";
	while (<$S>) {
		$answer = $1 if /^Sites (\d+)\s*$/;
		print "[".scalar(localtime)."] Crawler reports $answer sites ".
			"in progress\n" if $DEBUG_CONN;
	}
	close ($S);
	return $answer;
} 

# kill crawler
# in: port to connect to (e.g. "localhost:1")
sub kill_crawler{
	my ($PORT) = @_;
	my $S = IO::Socket::INET->new( "PeerHost" => $PORT ,
					 "Proto" => "tcp" );
	return -1 unless $S;
	print $S "DIE\r\n\r\n";
	close ($S);
	return 1;
}

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

# sql query until a site becomes available or none remain
sub getsite {
	my $getsite_tries = 0;
	my $nexthid;
	my ($host, $ip, $aliases, $maxpages, $maxdepth);
	my (@rooturls);

	getsite_begin:
	++$getsite_tries;
	# Hack: Do some backoff in case of transaction conflict.
	sleep ((rand) < 0.5 ? 0 : 1) if $getsite_tries > 1;
	# Panic if error is repeated; sleep to slow overflowing log, beating db
	if ($getsite_tries % 5 == 0) {
		# Panic
		# $DB_CONN might be undef, so we use DBI::errstr.
		warn "Repeatedly failed to get new site; last error: ".
			$DBI::errstr."\n";
		warn "Sleeping 10 seconds before trying again.\n";
		sleep 10;
	}
	undef $nexthid;
	undef $host;
	undef $ip;
	undef $aliases;
	undef $maxpages;
	undef $maxdepth;

	&connect() || goto getsite_begin;
	$DB_CONN->rollback();  # clear whatever transaction was in progress

	# if no sites remain, quit
	my @answer = 
	  &query("SELECT COUNT(*) FROM uncrawled");
#	  &query("SELECT COUNT(*) FROM \
#	  (SELECT * FROM ${DB_TABLE} WHERE status = 'uncrawled' LIMIT 1) AS t");
	scalar(@answer) || goto getsite_begin;
	return undef if ($answer[0] == 0);
	print "[".scalar(localtime)."] Sites remain to crawl\n" if $DEBUG_DB;

	# try to find a site
	@answer = &query("SELECT hid,ipaddr FROM uncrawled WHERE ipaddr NOT IN \
				(SELECT ipaddr FROM crawling) \
			  ORDER BY hid LIMIT 1");
#        @answer = &query("SELECT h.hid FROM Hosts h, ${DB_TABLE} s \
#                        WHERE h.hid = s.hid AND s.status = 'uncrawled' \
#                        AND (SELECT ipaddr from IPAddrs
#                                WHERE hid = h.hid
#                                ORDER BY ipaddr LIMIT 1) NOT IN
#                            (SELECT i.ipaddr from IPAddrs i, crawlcontrol c
#                                WHERE i.hid = c.hid AND c.status = 'crawling')
#			ORDER BY priority DESC LIMIT 1");
#
#                              AND NOT EXISTS \
#                        (SELECT t.status FROM ${DB_TABLE} t \
#                         WHERE t.status = 'crawling' AND \
#                               (SELECT ipaddr from IPAddrs \
#                                WHERE hid = h.hid \
#                                ORDER BY ipaddr LIMIT 1) = \
#                               (SELECT ipaddr from IPAddrs \
#                                WHERE hid = t.hid \
#                                ORDER BY ipaddr LIMIT 1) ) \
#                         ORDER BY priority DESC LIMIT 1");
	# scalar(@answer) || goto getsite_begin;
	# special case: sites remain, but ip conflict.  give up for now,
	# instead of repeatedly retry for it (in case of self-deadlock on ip).
	scalar(@answer) || return undef;
	$nexthid = $answer[0];
	$ip = $answer[1];
	print "[".scalar(localtime)."] SELECT: Claim hid $nexthid\n" if $DEBUG_DB;
		
	# try to claim it
	$DB_CONN->do("DELETE FROM uncrawled WHERE hid = $nexthid") ||
		goto getsite_begin;
	$DB_CONN->do("INSERT INTO crawling (hid,id,ipaddr) VALUES \
			($nexthid, $ID, '$ip')") || goto getsite_begin;
#	$DB_CONN->do("UPDATE ${DB_TABLE} SET status = 'crawling', id = $ID \
#			WHERE hid = $nexthid") || goto getsite_begin;

	@answer = &query("SELECT hostname,maxpages,maxdepth,minpause,rooturls ".
			"FROM Hosts \
			WHERE hid = $nexthid");
	scalar(@answer) || goto getsite_begin;
	$host = $answer[0];
	$maxpages = $answer[1];
	$maxdepth = $answer[2];
	$minpause = $answer[3];
	@rooturls = split(/\s+/,$answer[4]) if defined($answer[4]);
	@rooturls = ( "http://$host/" ) unless scalar(@rooturls);
#	@answer = &query("SELECT ipaddr FROM IPAddrs \
#			WHERE hid = $nexthid LIMIT 1");
#	scalar(@answer) || goto getsite_begin;
#	$ip = $answer[0];
	print "[".scalar(localtime)."] SELECT: $nexthid $host ($ip) ".
		"crawl $maxpages pages to depth $maxdepth from ".
		join(' ',@rooturls)."\n" if $DEBUG_DB;

	my $sth = $DB_CONN->prepare("SELECT alias FROM Aliases \
			WHERE hid = $nexthid") || goto getsite_begin;
	$sth->execute() || goto getsite_begin;
	print "[".scalar(localtime)."] SELECT: Aliases query execute\n"
		if $DEBUG_DB;

	$aliases = '';
	if ($sth->rows()) {
		my @alias = $sth->fetchrow_array();
		if (scalar(@alias)) {
			$aliases = $alias[0];
			# print "[".scalar(localtime)."] SELECT: ".
			#	"Alias >$aliases<\n" if $DEBUG_DB;

			while ( scalar(@alias = $sth->fetchrow_array()) ) {
				$aliases .= " ".$alias[0];
			}
		}
	}
	print "[".scalar(localtime)."] SELECT: $nexthid Aliases: $aliases\n"
		if $DEBUG_DB;

	$DB_CONN->commit() || goto getsite_begin;

	return ($host,$ip,$aliases,$maxpages,$maxdepth,$minpause,@rooturls);
}

# mark a crawling site with some other (completed) status
sub releasesite {
	my($site, $status) = @_;
	my $untainted = 0;

	print "[".scalar(localtime)."] releasesite: >$site< >$status<\n"
		if $DEBUG_DB;

	my $releasesite_tries = 0;
	releasesite_begin:
	++$releasesite_tries;
	# Hack: Do some backoff in case of transaction conflict.
	sleep ((rand) < 0.5 ? 0 : 1) if $releasesite_tries > 1;
	# Panic if error is repeated; sleep to slow overflowing log, beating db
	if ($releasesite_tries % 5 == 0) {
		# Panic
		warn "Repeatedly failed to relinquish site; last error: ".
			$DBI::errstr."\n";
		warn "Sleeping 10 seconds before trying again.\n";
		sleep 10;
	}

	&connect() || goto releasesite_begin;

	# untaint the input exactly once; quote it
	if ($untainted == 0) {
		$site = $DB_CONN->quote($site);
		$status = $DB_CONN->quote($status);
		$untainted = 1;
	}

	my $hid = -1;
	@answer = &query("SELECT hid FROM Hosts WHERE hostname = $site");
	scalar(@answer) || goto releasesite_begin;
	$hid = $answer[0];

	$DB_CONN->do("DELETE FROM crawling WHERE hid = $hid") ||
		goto getsite_begin;
	$DB_CONN->do("INSERT INTO crawled (hid,id,status) VALUES \
			($hid, $ID, $status)") || goto getsite_begin;
#	print("UPDATE ${DB_TABLE} SET status = $status, id = $ID \
#			WHERE hid = $hid\n") if $DEBUG_DB;
#	$DB_CONN->do("UPDATE ${DB_TABLE} SET status = $status, id = $ID \
#			WHERE hid = $hid") || goto releasesite_begin;

	$DB_CONN->commit() || goto releasesite_begin;

	return;
}

# send a CRAWL command to crawl_server
sub request_crawl_debug {
	my($port,$sitename,$ip,$aliases,$maxpages,$maxdepth,$minpause,
		@rooturls) = @_;
	# With a CRAWL command, @rooturls is not supported
	# (crawling always starts from /)
	@rooturls = ();
	print "[".scalar(localtime)."] CRAWL\r\n".
	      "$sitename $aliases\r\n$ip\r\n".
	      "$maxdepth $maxpages $minpause\r\n\r\n";
	return 1;
}
	
sub request_crawl {
# sub request_crawl_real {
	my($port,$sitename,$ip,$aliases,$maxpages,$maxdepth,$minpause,
		@rooturls) = @_;

	my $S = IO::Socket::INET->new( "PeerHost" => $port ,
				 	"Proto" => "tcp" );
	# Normal way:
	# print $S "CRAWL\r\n".
	#    "$sitename $aliases\r\n$ip\r\n$maxdepth $maxpages $minpause\r\n\r\n";

	# Hokey hack way:
	print $S "LOAD\r\nQueueInfoStart\nSiteNamesStart\n$sitename\n";
	print $S join("\n",split(/\s+/,$aliases))."\n" if $aliases;
	print $S "SiteNamesEnd\nSiteIpsStart\n$ip\nSiteIpsEnd\n";
	print $S "UrlQueueStart\nUrlsSeenStart\n1048576\nUrlsSeenEnd\n";
	print $S "UrlsToCrawlStart\n";
	print $S "http://$sitename/robots.txt 0 0\n";
	foreach (@rooturls) { 
		print $S "$_ 1 0\n";
	}
	print $S "UrlsToCrawlEnd\nUrlQueueEnd\nUrlFilterStart\n";
	print $S "SeedFilterStart\n1 $maxdepth $maxpages\nSeedFilterEnd\n";
	print $S "UrlFilterEnd\nUrlsPerSeedStart\n0\n0\nUrlsPerSeedEnd\n";
	print $S "DelayStart\n$minpause\nDelayEnd\nErrorStart\n0\nErrorEnd\n";
	print $S "QueueInfoEnd\n";
	print $S "\r\n\r\n";
	# All this to support @rooturls...  :)

	while(<$S>) {
		return 1 if /^Status:OK$/;
	}
	close($S);
	return undef;
}

