#!/usr/bin/perl -w
#
# crawl_buddy.pl - control a crawl_server (prototype)
# Wang Lam <wlam@cs.stanford.edu>
# June 2002
# Last updated October 2003
#
# crawl_buddy.pl replaces the siteserver + crawl_servers + starter setup
# with a database + (crawl_buddy.pl+crawl_server) setup.
#
# Each crawl_buddy.pl spawns a crawl_server, then feeds it sites
# from a database, acting as the crawl_server's "siteserver" (coordinator).
# In this setup, siteserver and starter are unneeded.  crawl_buddy.pl
# terminates crawl_server and itself automatically when all sites in the
# database are crawled.  crawl_buddy.pl automatically dies if its crawl_server
# dies.  (In this case, clean up database information for the crawler and
# restart crawl_buddy.pl to continue; crawl state is stored in a database,
# not a siteserver.)
#
# Usage:
# crawl_binaries> ./crawl_buddy.pl -id [nonnegative integer] [-v[erbose]]
#
# To stop a crawl_buddy most gracefully, crawl_ctl.pl winddown crawler-id
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

$LOG_PREFIX = "log/crawl_buddy.log.";	# log error messages

$DEBUG_STAT = "starting up";		# the current operation being conducted
$DEBUG_STAT_SUB = "";			# the current status of a subroutine

$SIG{TERM} = sub {
	die "Terminating: current operation - $DEBUG_STAT - $DEBUG_STAT_SUB\n";
};

## more globals

sub read_config {
	# Read crawl_config; fill %%CONFIG_VAR. (taken from crawl_ctl.pl)
	$config_file = "crawl_config";
	open(CONFIG,"<$config_file") || return undef;
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
	return 1;
}

&read_config || die "Cannot open crawl_config: $!\n";

$ID = -1;				# crawler ID
	Getopt::Long::GetOptions("id=i" => \$ID,
				 "verbose" => \$VERBOSE);
($LISTEN_PORT,$CRAWLER_PORT) =		# $0's, crawl_server's listen ports
	&getports($ID);
$CRAWL = 1;				# to crawl or not to crawl
$CONCURRENT_SITES = $CONFIG_VAR{'starter'}{'Threads'};
					# max concurrent sites for crawl_server
$LOG_NAME = "$LOG_PREFIX$ID";		# error log

$DB = $CONFIG_VAR{'siteserver'}{'db'};	# name of database to use
$DB_USER = 				# database username
	$CONFIG_VAR{'siteserver'}{'dbuser'};
$DB_PASSWD = 				# database username password
	$CONFIG_VAR{'siteserver'}{'dbpasswd'};
$DB_CONN = undef;			# handle to database

## main loop

die "Usage: $0 -id <crawler id> [-v[erbose]]\n" unless $ID >= 0;
if (! $VERBOSE) {
	open (LOG, ">>$LOG_NAME") ||
		die "Cannot open log file $LOG_NAME: $!\n";
	select(LOG);  $| = 1;  # Quell "LOG used only once" warning
};

# Check for a corresponding crawl_server
$DEBUG_STAT = "checking for crawl_server";
if (&status($CRAWLER_PORT) >= 0) {
	die "crawl_server already running on $CRAWLER_PORT ".
		"or port already taken!";
};

# spawn crawl_server
# too many spurious CHLD signals: just let &status fail
$SIG{CHLD} = sub { wait; }; # die "crawl_server died; aborting.\n"; };
print "[".scalar(localtime)."] Starting crawl_server...\n" if $VERBOSE;
$DEBUG_STAT = "forking crawl_server";
unless ($CRAWLER_PID = fork()) {
	open(STDOUT,">>/dev/null");
	open(STDERR,">>/dev/null");
	$DEBUG_STAT = "execing crawl_server";
	exec(split(/ /,"./crawl_server -id $ID -l $CRAWLER_PORT -s $LISTEN_PORT")) || die "Can't exec ./crawl_server: $!\n";
}
$DEBUG_STAT = "waiting for crawl_server to start up";
sleep 10;  # crawl_server needs time to set up; we can be conservative here...
$DEBUG_STAT = "checking for new crawl_server";
die "./crawl_server failed, cannot continue.\n"
	unless &status($CRAWLER_PORT) >= 0;

# Listen
$DEBUG_STAT = "setting up listen socket";
$LS = IO::Socket::INET->new( "LocalAddr" => $LISTEN_PORT,
			     "Proto"	 => "tcp",
			     "Listen"	 => 300  		);
					  # MAX_NUM_QUEUES 
die "Can't create listen socket: $!\n" unless $LS;
$LS->listen() || die "Can't listen on $LISTEN_PORT: $!\n";

# Daemonize
$DEBUG_STAT = "daemonizing";
if ($VERBOSE) {
	STDOUT->autoflush(1);
	STDERR->autoflush(1);
} else {
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

$LOG_STOPNEW = 0; # STOPNEW crawling received?
$LOG_CRAWLNEW = 0; # CRAWLNEW sites (undo STOPNEW) received?
# reread config not really supported: what if ports or db changes?
$LOG_RECONFIG = 0;
$SIG{HUP} = sub { $LOG_RECONFIG = 1; };
# a toggle to turn on and off debugging information
# should be SIGINFO, for operating systems that have it
$SIG{USR1} = sub {
		$DEBUG_CONN = $DEBUG_CONN ? 0 : 1; 
		$DEBUG_DB = $DEBUG_DB ? 0 : 1;
		};

# Starter
$DEBUG_STAT = "logging start of crawl";
print "[".scalar(localtime)."] Beginning crawl of $CONCURRENT_SITES ".
	"maximum concurrent sites...\n";
print "[".scalar(localtime)."] Information for crawl_server: ".
	"Port $CRAWLER_PORT PID $CRAWLER_PID\n";
print "[".scalar(localtime)."] Information for crawl_buddy.pl: ".
	"Port $LISTEN_PORT PID $$\n";
# while (&status($CRAWLER_PORT) < $CONCURRENT_SITES) {
$DEBUG_STAT = "checking crawler to request first crawl";
if (&status($CRAWLER_PORT) < $CONCURRENT_SITES) {
	last if !$CRAWL;
	$DEBUG_STAT = "finding first site to crawl";
	my ($sitename,$ip,$aliases,$maxpages,$maxdepth,$minpause,@rooturls) =
		&getsite();
	if (!defined($sitename)) {
		$DEBUG_STAT = "no first site to crawl";
		print "[".scalar(localtime)."] No more sites available to crawl.\n";
		# last;  # give up trying to add more sites
	} else {
		$DEBUG_STAT = "requesting first site to crawl";
		print "[".scalar(localtime)."] Requesting crawl $sitename\n";
		&request_crawl($CRAWLER_PORT,$sitename,$ip,$aliases,
			$maxpages,$maxdepth,$minpause,@rooturls);
	}
	# $LS->timeout(1);
	$LS->timeout(30);	# One sec is a reasonable timeout,
				# except on Linux. :P
}

# Main loop
print "[".scalar(localtime)."] Starting main listen loop\n" if $DEBUG_CONN;
while(1) {
	$DEBUG_STAT = "waiting for incoming message connection";
	my $CS = $LS->accept();
	# $LS->timeout(1);
	$LS->timeout(30);	# One sec is a reasonable timeout,
				# except on Linux. :P
	# next unless defined($CS);
	$DEBUG_STAT = "received connection for message";
	if (defined($CS)) {
	# $CS->timeout(10);
	$CS->timeout(30);	# Ten sec may seem an excessive timeout,
				# except on Linux. :P

	# Get report of finished site
	my $site = '';
	my $endstatus = '';
	my $fetched = -1;
	my $dummyreport = 0;
	my $cmd = '';
	my $cmdvalid = 0;
	$DEBUG_STAT = "reading line for incoming message";
	while (<$CS>) {
		# should, but doesn't verify the message is Type:Report
		# should, but doesn't verify that CrawlerId matches db
		print "[".scalar(localtime)."] Message: $_" if $DEBUG_CONN;
		$dummyreport=1		if /^Status: Dummy$/;
		$site = $1		if /^SiteName: (.*)$/;
		$endstatus = $1		if /^Status: (.*)$/;
		$fetched = $1		if /^Fetched: (.*)$/;
		# (print "End message\n", last) unless /\w/;
		$DEBUG_STAT =
			"reading line for incoming message (last line $_)";

		$cmd = 'die', last	if /^DIE/;
		$cmd = 'stopnew', last	if /^STOPNEW/;
		$cmd = 'crawlnew', last	if /^CRAWLNEW/;
	}
	$DEBUG_STAT = "checking incoming message type";
	if ($cmd eq 'die') {
		# Drop what we're doing, kill crawler and die.
		$cmdvalid = 1;
		close($CS);
		last;
	} elsif ($cmd eq 'stopnew' ) {
		# Stop picking up new sites, but finish crawling.
		$cmdvalid = 1;
		$LOG_STOPNEW = 1;
	} elsif ($cmd eq 'crawlnew' ) {
		# Pick up new sites again (undo 'stopnew').
		$cmdvalid = 1;
		$LOG_CRAWLNEW = 1;
	}

	$DEBUG_STAT = "replying to incoming crawler message";
	print $CS "Status:OK\r\n\r\n";
	close($CS);
	$site && $endstatus && ($fetched != -1) or $cmdvalid or 
		warn "Malformed incoming site-done message or command.\n";

	# Update status for finished site
	if (!$dummyreport and $site and $endstatus) {
		chomp $site;
		chomp $endstatus;
		print "[".scalar(localtime)."] Finished crawl $site: Fetched $fetched: $endstatus\n";
		$DEBUG_STAT = "marking site $site done";
		&releasesite($site, $fetched, $endstatus);
	}

	} # if defined($CS)

        if ($LOG_RECONFIG) {
		print "[".scalar(localtime).
			"] SIGHUP received: ";
		$DEBUG_STAT = "responding to SIGHUP by rereading config";
		if (&read_config) {
			$CONCURRENT_SITES = $CONFIG_VAR{'starter'}{'Threads'};
			print "resetting starter|Threads to $CONCURRENT_SITES";
		} else {
			print "cannot reread configuration file: $!";
		}
		print "\n";
		$LOG_RECONFIG = 0;
	}

	# Optional "Should I stop crawling?" decision-making goes here. #
	$DEBUG_STAT = "checking for crawler abort or loss";
	if (&status($CRAWLER_PORT) == -1) {
		print "[".scalar(localtime)."] Crawler lost, aborting.\n";
		last;
	}
	$DEBUG_STAT = "checking for crawl_buddy stop-request signals";
	if ($LOG_CRAWLNEW) {
		print "[".scalar(localtime)."] New crawling resumed.\n";
		$CRAWL = 1;
		$LOG_CRAWLNEW = 0;
	}
	if ($LOG_STOPNEW) {
		print "[".scalar(localtime)."] New crawling halted.\n";
		$CRAWL = 0;
		$LOG_STOPNEW = 0;
	}

	# Get a new site to replace finished site
	my ($sitename,$ip,$aliases,$maxpages,$maxdepth,@rooturls) = undef;
	$DEBUG_STAT = "checking if crawl_server can accept new site";
	if ($CRAWL && (&status($CRAWLER_PORT) < $CONCURRENT_SITES)) {
		$DEBUG_STAT = "finding new site to crawl";
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
		$DEBUG_STAT = "requesting crawl of new site";
		&request_crawl($CRAWLER_PORT,$sitename,$ip,$aliases,
			$maxpages,$maxdepth,$minpause,@rooturls);
		$LS->timeout(1);
		$endcrawl_delay = 0;
	} else {
		# Wait until all sites being crawled are finished.
		$DEBUG_STAT = "checking of crawl_server has finished sites";
		if (&status($CRAWLER_PORT) <= 0) {
			++$endcrawl_delay;
			print "[".scalar(localtime)."] ".
				"Crawler KO count: $endcrawl_delay\n"
				if $DEBUG_CONN;
		} else {
			$endcrawl_delay = 0;
		}
		last if $endcrawl_delay > 5;
	}
}

# exit when done (or should this be while(1) getsite()?)
$DEBUG_STAT = "ending crawl";
print "[".scalar(localtime)."] Crawl finished: ".
	"cleaning up database and crawl_server\n";
$DEBUG_STAT = "disconnecting from database";
$DB_CONN->disconnect();
print "[".scalar(localtime)."] Kill crawler: " if $DEBUG_CONN;
$DEBUG_STAT = "ending crawl_server process";
my $retvalue = &kill_crawler($CRAWLER_PORT);
print "$retvalue\n" if $DEBUG_CONN;
$DEBUG_STAT = "exiting crawl_buddy";
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
	$DEBUG_STAT_SUB = "opening connection to crawl_server";
	my $S = IO::Socket::INET->new( "PeerHost" => $PORT ,
					 "Proto" => "tcp" );
	return -1 unless $S;
	my $answer = -1;
	$DEBUG_STAT_SUB = "sending STATUS request to crawl_server";
	print $S "STATUS\r\n\r\n";
	$DEBUG_STAT_SUB = "reading STATUS reply from crawl_server";
	while (<$S>) {
		$DEBUG_STAT_SUB =
			"reading STATUS reply from crawl_server (last line $_)";
		$answer = $1 if /^Sites (\d+)\s*$/;
		print "[".scalar(localtime)."] Crawler reports $answer sites ".
			"in progress\n" if $DEBUG_CONN;
	}
	$DEBUG_STAT_SUB = "closing connection to crawl_server";
	close ($S);
	$DEBUG_STAT_SUB = "";
	return $answer;
} 

# kill crawler
# in: port to connect to (e.g. "localhost:1")
sub kill_crawler{
	my ($PORT) = @_;
	$DEBUG_STAT_SUB = "opening connection to crawl_server";
	my $S = IO::Socket::INET->new( "PeerHost" => $PORT ,
					 "Proto" => "tcp" );
	return -1 unless $S;
	$DEBUG_STAT_SUB = "sending DIE request to crawl_server";
	print $S "DIE\r\n\r\n";
	$DEBUG_STAT_SUB = "closing connection to crawl_server";
	close ($S);
	$DEBUG_STAT_SUB = "";
	return 1;
}

# A quick one-line 'ensure db is connected'
sub connect {
	print "[".scalar(localtime)."] check connect: " if $DEBUG_DB;
	$DEBUG_STAT_SUB = "checking that DB connection is active and pingable";
	if (defined($DB_CONN) and (!$DB_CONN->{Active} or !$DB_CONN->ping())) {
		print "broken, disconnecting; " if $DEBUG_DB;
		$DEBUG_STAT_SUB = "disconnecting broken DB connection";
		$DB_CONN->disconnect();
		$DB_CONN = undef;
	}
	if (!defined($DB_CONN)) {
		print "connecting: " if $DEBUG_DB;
		$DEBUG_STAT_SUB = "connecting to DB";
		$DB_CONN = DBI->connect("DBI:Pg:dbname=$DB",$DB_USER,$DB_PASSWD,
			{ AutoCommit => 0 });
	}
	$DEBUG_STAT_SUB = "checking DB connect was successful";
	if (defined($DB_CONN)) { 
		print "connected.\n" if $DEBUG_DB;
	} else {
		print "unconnected.\n" if $DEBUG_DB;
	}
	$DEBUG_STAT_SUB = "";
	return $DB_CONN;
}

# A quick one-line query for a one-line answer
# Returns list with answer, or () (not undef!) if not possible.
sub query {
	my ($sql) = @_;
	print "[".scalar(localtime)."] query: prepare " if $DEBUG_DB;
	$DEBUG_STAT_SUB = "asking DB to prepare SQL query ($sql)";
	my $sth = $DB_CONN->prepare($sql);
	if (!defined($sth)) {
		print "failed.\n" if $DEBUG_DB;
		return undef;
	}
	print "- execute " if $DEBUG_DB;
	$DEBUG_STAT_SUB = "asking DB to execute SQL query ($sql)";
	if (!$sth->execute()) {
		print "failed.\n" if $DEBUG_DB;
		return undef;
	}
	print "- ready to fetch.\n" if $DEBUG_DB;
	$DEBUG_STAT_SUB = "checking for rows in answer to SQL query ($sql)";
	return () unless $sth->rows();

	$DEBUG_STAT_SUB = "fetching row for answer to SQL query ($sql)";
	return $sth->fetchrow_array();
	# Oops, no chance to clean up $DEBUG_STAT_SUB here.
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
	$DEBUG_STAT_SUB = "sleeping up to 3 seconds to back off";
	sleep ((rand) * 3) if $getsite_tries > 1;
	# Panic if error is repeated; sleep to slow overflowing log, beating db
	if ($getsite_tries % 5 == 0) {
		# Panic
		# $DB_CONN might be undef, so we use DBI::errstr.
		my $sleeptime = 10 + ((rand) * 10);
		warn "Repeatedly failed to get new site; last error: ".
			$DBI::errstr."\n";
		warn "Sleeping $sleeptime seconds before trying again.\n";
		$DEBUG_STAT_SUB = "sleeping $sleeptime seconds to back off";
		sleep $sleeptime;
	}
	undef $nexthid;
	undef $host;
	undef $ip;
	undef $aliases;
	undef $maxpages;
	undef $maxdepth;

	$DEBUG_STAT_SUB = "ensuring connection to database";
	&connect() || goto getsite_begin;
	$DEBUG_STAT_SUB = "roll back any transaction in progress";
	$DB_CONN->rollback();  # clear whatever transaction was in progress
	# PostgreSQL and Oracle allow the following non-SQL92 command,
	# which may help prevent contention for the same row of uncrawled:
	$DB_CONN->do("LOCK TABLE uncrawled IN SHARE ROW EXCLUSIVE MODE");
	# Note that SHARE ROW EXCLUSIVE conflicts with another of the same,
	# but ROW EXCLUSIVE does not conflict with another ROW EXCLUSIVE!

	# if no sites remain, quit
	$DEBUG_STAT_SUB = "look for uncrawled sites";
	my @answer = 
	  &query("SELECT COUNT(*) FROM uncrawled");
#	  &query("SELECT COUNT(*) FROM \
#	  (SELECT * FROM ${DB_TABLE} WHERE status = 'uncrawled' LIMIT 1) AS t");
	scalar(@answer) || goto getsite_begin;
	return undef if ($answer[0] == 0);
	print "[".scalar(localtime)."] Sites remain to crawl\n" if $DEBUG_DB;

	# try to find a site
	$DEBUG_STAT_SUB = "select an uncrawled site";
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
	$DEBUG_STAT_SUB = "claim an uncrawled site by deleting from uncrawled";
	$DB_CONN->do("DELETE FROM uncrawled WHERE hid = $nexthid") ||
		goto getsite_begin;
	$DEBUG_STAT_SUB = "claim an uncrawled site by inserting into crawling";
	$DB_CONN->do("INSERT INTO crawling (hid,id,ipaddr) VALUES \
			($nexthid, $ID, '$ip')") || goto getsite_begin;
#	$DB_CONN->do("UPDATE ${DB_TABLE} SET status = 'crawling', id = $ID \
#			WHERE hid = $nexthid") || goto getsite_begin;

	$DEBUG_STAT_SUB = "get parameters for new uncrawled site";
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

	$DEBUG_STAT_SUB = "prepare SQL query to get aliases for new site";
	my $sth = $DB_CONN->prepare("SELECT alias FROM Aliases \
			WHERE hid = $nexthid") || goto getsite_begin;
	$DEBUG_STAT_SUB = "execute SQL query to get aliases for new site";
	$sth->execute() || goto getsite_begin;
	print "[".scalar(localtime)."] SELECT: Aliases query execute\n"
		if $DEBUG_DB;

	$aliases = '';
	$DEBUG_STAT_SUB = "check for rows in query to get aliases for new site";
	if ($sth->rows()) {
		$DEBUG_STAT_SUB = "fetch alias of new site";
		my @alias = $sth->fetchrow_array();
		if (scalar(@alias)) {
			$aliases = $alias[0];
			# print "[".scalar(localtime)."] SELECT: ".
			#	"Alias >$aliases<\n" if $DEBUG_DB;

			$DEBUG_STAT_SUB =
				"fetch alias of new site (last $alias[0])";
			while ( scalar(@alias = $sth->fetchrow_array()) ) {
				$aliases .= " ".$alias[0];
			}
		}
	}
	print "[".scalar(localtime)."] SELECT: $nexthid Aliases: $aliases\n"
		if $DEBUG_DB;

	$DEBUG_STAT_SUB = "commit claim of new site";
	$DB_CONN->commit() || goto getsite_begin;

	$DEBUG_STAT_SUB = "";
	return ($host,$ip,$aliases,$maxpages,$maxdepth,$minpause,@rooturls);
}

# mark a crawling site with some other (completed) status
sub releasesite {
	my($site, $fetched, $status) = @_;
	my $untainted = 0;

	print "[".scalar(localtime)."] releasesite: ".
		">$site< >$fetched< >$status<\n" if $DEBUG_DB;

	my $releasesite_tries = 0;
	releasesite_begin:
	++$releasesite_tries;
	# Hack: Do some backoff in case of transaction conflict.
	$DEBUG_STAT_SUB = "sleeping up to 3 seconds to back off";
	sleep ((rand) * 3) if $releasesite_tries > 1;
	# Panic if error is repeated; sleep to slow overflowing log, beating db
	if ($releasesite_tries % 5 == 0) {
		# Panic
		my $sleeptime = 10 + ((rand) * 10);
		warn "Repeatedly failed to relinquish site; last error: ".
			$DBI::errstr."\n";
		warn "Sleeping $sleeptime seconds before trying again.\n";
		$DEBUG_STAT_SUB = "sleeping $sleeptime seconds to back off";
		sleep $sleeptime;
	}

	$DEBUG_STAT_SUB = "ensuring connection to database";
	&connect() || goto releasesite_begin;
	$DEBUG_STAT_SUB = "roll back any transaction in progress";
	$DB_CONN->rollback();  # clear whatever transaction was in progress

	# untaint the input exactly once; quote it
	if ($untainted == 0) {
		$DEBUG_STAT_SUB = "untaint site and status string to record";
		$site = $DB_CONN->quote($site);
		$status = $DB_CONN->quote($status);
		$untainted = 1;
	}

	my $hid = -1;
	$DEBUG_STAT_SUB = "select hid for site being released";
	@answer = &query("SELECT hid FROM Hosts WHERE hostname = $site");
	scalar(@answer) || goto releasesite_begin;
	$hid = $answer[0];

	$DEBUG_STAT_SUB = "delete site from crawling";
	$DB_CONN->do("DELETE FROM crawling WHERE hid = $hid") ||
		goto releasesite_begin;
	$DEBUG_STAT_SUB = "insert site into crawled";
	$DB_CONN->do("INSERT INTO crawled (hid,id,fetched,status) VALUES \
			($hid, $ID, $fetched, $status)") || 
			goto releasesite_begin;
#	print("UPDATE ${DB_TABLE} SET status = $status, id = $ID \
#			WHERE hid = $hid\n") if $DEBUG_DB;
#	$DB_CONN->do("UPDATE ${DB_TABLE} SET status = $status, id = $ID \
#			WHERE hid = $hid") || goto releasesite_begin;

	$DEBUG_STAT_SUB = "commit site release transaction";
	$DB_CONN->commit() || goto releasesite_begin;

	$DEBUG_STAT_SUB = "";
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

	my $S = undef;
	my $attempts = 0;
	print "[".scalar(localtime)."] request_crawl connect: " if $DEBUG_CONN;
	while (!defined($S)) {
		$DEBUG_STAT_SUB = "attempt connection to crawl_server";
     	  	$S = IO::Socket::INET->new( "PeerHost" => $port ,
				 	"Proto" => "tcp" );
		$DEBUG_STAT_SUB = "checking connection to crawl_server";
		if (defined($S)) { print "." if $DEBUG_CONN; }
		else { print "!" if $DEBUG_CONN; };
		++$attempts;
		if ($attempts > 5) {
			warn "Could not reach crawler; could not crawl site $sitename: $!\n";
			$DEBUG_STAT_SUB = "";
			return undef;
		}
	}
	print " command: " if $DEBUG_CONN;

	# Normal way:
	# print $S "CRAWL\r\n".
	#    "$sitename $aliases\r\n$ip\r\n$maxdepth $maxpages $minpause\r\n\r\n";

	# Hokey hack way:
	$DEBUG_STAT_SUB = "sending LOAD request to crawl_server";
	print $S "LOAD\r\nQueueInfoStart\nSiteNamesStart\n$sitename\n";
	print "." if $DEBUG_CONN;
	print $S join("\n",split(/\s+/,$aliases))."\n" if $aliases;
	print "." if $DEBUG_CONN;
	print $S "SiteNamesEnd\nSiteIpsStart\n$ip\nSiteIpsEnd\n";
	print "." if $DEBUG_CONN;
	print $S "UrlQueueStart\n";
	print "." if $DEBUG_CONN;
	print $S "UrlsToCrawlStart\n";
	print "." if $DEBUG_CONN;
	print $S "http://$sitename/robots.txt 0 0\n";
	print "." if $DEBUG_CONN;
	foreach (@rooturls) { 
		print $S "$_ 1 0\n";
		print "." if $DEBUG_CONN;
	}
	print $S "UrlsToCrawlEnd\n";
	print "." if $DEBUG_CONN;
	print $S "UrlsSeenStart\n1048576\nUrlsSeenEnd\n";
	print "." if $DEBUG_CONN;
	print $S "UrlQueueEnd\nUrlFilterStart\n";
	print "." if $DEBUG_CONN;
	print $S "SeedFilterStart\n1 $maxdepth $maxpages\nSeedFilterEnd\n";
	print "." if $DEBUG_CONN;
	print $S "UrlFilterEnd\nUrlsPerSeedStart\n0\n0\nUrlsPerSeedEnd\n";
	print "." if $DEBUG_CONN;
	print $S "DelayStart\n$minpause\nDelayEnd\nErrorStart\n0\nErrorEnd\n";
	print "." if $DEBUG_CONN;
	print $S "QueueInfoEnd\n";
	print "." if $DEBUG_CONN;
	print $S "\r\n\r\n";
	print ". receive: " if $DEBUG_CONN;
	# All this to support @rooturls...  :)
	
	$DEBUG_STAT_SUB = "reading reply to LOAD request";
	while(<$S>) {
		if (/^Status:OK$/) {
			print ".\n" if $DEBUG_CONN;
			$DEBUG_STAT_SUB = "closing connection to crawl_server";
			close($S);
			$DEBUG_STAT_SUB = "";
			return 1;
		} else {
			print "!" if $DEBUG_CONN;
		}
	}
	$DEBUG_STAT_SUB = "closing connection to crawl_server";
	close($S);
	print " return undef.\n" if $DEBUG_CONN;
	$DEBUG_STAT_SUB = "";
	return undef;
}

