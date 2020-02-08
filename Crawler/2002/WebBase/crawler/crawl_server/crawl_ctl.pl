#!/usr/bin/perl -w
#
# crawlerctl - crawler control
# Wang Lam <wlam@cs.stanford.edu>, Mar 2002
# edited Jun 2002 to use crawl_buddy.pl
#
# requires crawl_servers compiled with #define SOCKET_CONTROL
# usage: cd crawl_binaries && crawlerctl.pl {start|pause|resume|status|end} id
#
#

use IO::Socket;

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
# For general use, we should pick up the config dirs too. :P

# Dispatch the command.
$command = defined($ARGV[0]) ? $ARGV[0] : "";
$crawler_id = defined($ARGV[1]) ? $ARGV[1] : "";

# switch construct (see perlsyn(1).)
SWITCH: foreach ($command) {
	if ($crawler_id !~ /^\d+$/ ) { goto USAGE; }

	/^start$/i	&& do { &start($crawler_id); last; };

	# { &sendcmd($crawler_id,$command); last; }
	/^pause$/i	&& do { &sendcmd($crawler_id,"PAUSE"); last; };
	/^resume$/i	&& do { &sendcmd($crawler_id,"RESUME"); last; };
	/^status$/i	&& do { &sendcmd($crawler_id,"STATUS"); last; };
	/^end$/i	&& do { &sendcmd($crawler_id,"DIE"); last; };

	USAGE:
	print "Usage: crawlerctl.pl {start|pause|resume|status|end} ".
		"crawler-id\n";
}

exit 0;

# Library of functions

# Start a crawl_server.
# So long as crawl_server doesn't daemonize, invoker's streams won't close.
# This may hang PHP's system(), for example, or ssh connections on close.
sub start_old {
	my($id) = @_;
	system("./crawl_server -id $id -daemon 1 >/dev/null") == 0 or
		die "Can't run ./crawl_server: $!\n";
	sleep 2;  # Let crawl_server open its listen socket.
	system("./starter -id $id >/dev/null &") == 0 or
		die "Can't run ./starter: $!\n";
	return;
}

sub start {
	my($id) = @_;
	system("perl ./crawl_buddy.pl -id $id") == 0 or
		die "Can't run ./crawl_buddy.pl: $!\n";
	return;
}

# To be more robust, sendcmd should take the commands that go to
# crawl_buddy, and cc them to crawl_server in case crawl_buddy is dead.
# Here, it only sends to one of crawl_buddy or crawl_server as appropriate,
# assuming crawl_buddy is around and will forward its command(s).
sub sendcmd {
	my($id,$cmd) = @_;
	my($host,$port);

	# DIE command must go to crawl_buddy.pl 
	if ($cmd eq 'DIE') {
		defined($CONFIG_VAR{"siteserver"}{"LocalAddress"}) ||
			die "Invalid $config_file: ".
				"[siteserver] LocalAddress not found\n";
		my(@ports) = split(',',$CONFIG_VAR{"siteserver"}{"LocalAddress"});
		($host,$port) = split(':',$ports[$id]);
	} else {
		defined($CONFIG_VAR{"crawlserver"}{"LocalAddress"}) ||
			die "Invalid $config_file: ".
				"[crawlserver] LocalAddress not found\n";
		my(@ports) = split(',',$CONFIG_VAR{"crawlserver"}{"LocalAddress"});
		($host,$port) = split(':',$ports[$id]);
	}

	my($S) =  IO::Socket::INET->new(PeerAddr => $host, PeerPort => $port,
		Proto => 'tcp');
	defined($S) || die "Cannot connect to crawler (crawler not running?)\n";
	$S->autoflush(1);

	print $S "$cmd\r\n\r\n";
	$_ = <$S>;
	
	# die doesn't return anything.
	if ($cmd ne "DIE") {
		if (/^Status: OK\r\n/ || /^OK\r\n/) {
			while(<$S>) {
				s/\r//;
				print $_;
			}
		} else {
			print "Crawler returns error status: $_";
			print <$S>;
		}
	}
	$S->close;
}

