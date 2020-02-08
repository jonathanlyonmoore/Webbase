<?php
/* crawllib.php - crawler status functions
   Wang Lam <wlam@cs.stanford.edu> March 2002
*/

/* By php "tradition," max line length=4096.  s/4096/newvalue/g
   as needed in this file. */

/* $crawler_run is a path to a crawl_binaries directory,
   crawl_config and crawl_configdir get filled with config info. */
/* Returns true if it was valid and read; false on error. */
function readconfig($crawler_run, &$config_var, &$config_crawldir)
{
	/* Read the WebBase crawler configuration file */
	$configfile = @fopen("$crawler_run/crawl_config","r");
	if (!$configfile) {
		return FALSE;
	}
	$section = "";
	while (!feof($configfile)) {
		$line = trim(fgets($configfile,4096));
		$matches = array();
		if (ereg( '^\[(.*)\]$' , $line , $matches )) {
			$section = $matches[1];
		} else if (ereg( '^([^#=][^=]*)=(.*)$' , $line , $matches )) {
			$config_var[$section][$matches[1]] = $matches[2];
		}
	}
	fclose($configfile);

	/* We used to have starter|Crawlers indicate how many crawlers max,
	   but without it, we must use the min() of the assigned ports. */
	$config_var['starter']['Crawlers'] = min(
		count(explode(",",$config_var['siteserver']['LocalAddress'])),
		count(explode(",",$config_var['crawlserver']['LocalAddress'])));

	/* Read the WebBase crawler path files */
	for ($i = 0; $i < $config_var['starter']['Crawlers']; ++$i) {
		$configfilename = $config_var['crawlserver']['ConfigFile'];
		$configfilename = "$crawler_run/$configfilename.$i";
		$configfile=@fopen($configfilename, "r");
		if (!$configfile) {
			return FALSE;
		}
		$line = rtrim(fgets($configfile,4096));
		/* Is the crawler really whitespace-insensitive with $line? */
		$config_crawldir[$i] = $line;
		fclose($configfile);
	}

	return TRUE;
}

/* crawler_id is an int such that
   0 <= $crawler_id < $config_var['crawlserver']['Crawlers'] */
/* ($crawler_id is not actually checked, though.) */
/* returns true if allruns and lastrun get filled with stats, false on error */
function parselog($crawler_run, $crawler_id, &$allruns, &$lastrun) {
	$logfilename = escapeshellarg($crawler_run)."/log/crawl_server.".
		escapeshellarg($crawler_id).".???)";
	$log = @popen("cat $logfilename","r");
	if ($log == false) {
		 return(FALSE);
	}

	$statall['numstarts'] = 0;
	$statall['starttime'] = "";
	$statall['numpages'] = 0;
	$statall['numbytes'] = 0;
	$statall['endtime'] = "";
	$statlast['starttime'] = "";
	$statlast['numpages'] = 0;
	$statlast['numbytes'] = 0;
	$statlast['endtime'] = "";

	$numlines = 0;
	while (!feof($log)) {
		$line = trim(fgets($log,4096));
		if (! ereg('^\[([0-9:/ ]+)\] ',$line,$timestamp))
			continue;
		$timestamp = $timestamp[1];
		++$numlines;
		# print "Message time $timestamp<BR>\n";

		if (ereg(' Starting Crawler ',$line)) {
			# print "$timestamp: crawler restart<BR>\n";

			++$statall['numstarts'];
			if ( $statall['starttime'] == 0 ) {
				$statall['starttime'] = $timestamp;
			}
			$statall['endtime'] = $timestamp;

			// reset last-invocation stats
			$statlast['numpages'] = 0;
			$statlast['numbytes'] = 0;
			$statlast['starttime'] = $timestamp;
			$statlast['endtime'] = $timestamp;
		} elseif (preg_match('/ Done: .*size:(\d+) \d+ ms\)$/',
				$line,$pgsize)) {
			# print "$timestamp: ${pgsize[1]}-byte page<BR>\n";
			if ( $statall['starttime'] == 0 ) {
				$statall['starttime'] = $timestamp;
			}
			if ( $statlast['starttime'] == 0 ) {
				$statall['starttime'] = $timestamp;
			}

			++$statall['numpages'];
			$statall['numbytes'] += $pgsize[1];

			++$statlast['numpages'];
			$statlast['numbytes'] += $pgsize[1];

			$statall['endtime'] = $timestamp;
			$statlast['endtime'] = $timestamp;
		}

	}
	pclose($log);

	if ($statall['starttime'] == "" or $statall['endtime'] == "") {
		return FALSE;
	}
	$statall['starttime'] = strtotime($statall['starttime']);
	$statall['endtime'] = strtotime($statall['endtime']);
	$statlast['starttime'] = strtotime($statlast['starttime']);
	$statlast['endtime'] = strtotime($statlast['endtime']);
	$allruns = $statall;
	$lastrun = $statlast;
	# foreach ($allruns as $k => $v) {
	#	print "$k => $v\n";
	# }
	return TRUE;
}

/* crawler_id is an int such that
   0 <= $crawler_id < $config_var['crawlserver']['Crawlers'] */
/* returns true if allruns and lastrun get filled with stats, false on error */
function statcrawler($crawler_run, $crawler_id, &$allruns, &$lastrun) {
	if (!readconfig($crawler_run, &$config_var, &$config_crawldir)) {
		# print "Read config...";
		return FALSE;
	}

	$timeout = 5;  /* max number of seconds we can consume per socket op */
	$ports = explode(",",$config_var['crawlserver']['LocalAddress']);
	list($hostname, $port) = explode(":",$ports[$crawler_id]);
	# print "Got port $hostname:$port...";
	$socket = fsockopen($hostname,$port,$errno,$errstr,$timeout);
	if ($socket == FALSE)
		return FALSE;

	# print "Requesting status...";
	socket_set_timeout($socket,$timeout,0);
	$command = "STATUS\r\n\r\n";
	if (fwrite($socket,$command,strlen($command)) == FALSE)
		return FALSE;
	fflush($socket);

	# print "Reading status...";
	$stats = fread($socket,4096);
	fclose($socket);
	# print "&gt;<PRE>$stats</PRE>&lt;";

	# sscanf($stats,"OK\r\nCrawler PID %d\r\nStart seconds %d\r\nStatus seconds %d\r\nBytes saved %d\r\nPages saved %d\r\nStatus %s\r\n",
	sscanf($stats,"OK\r\nCrawler PID %s\r\nStart seconds %s\r\nStatus seconds %s\r\nBytes saved %s\r\nPages saved %s\r\nSites %d\r\nStatus %s\r\n",
			&$pid,
			&$allruns['starttime'],
			&$allruns['endtime'],
			&$allruns['numbytes'],
			&$allruns['numpages'],
			&$allruns['numsites'],
			$status);
		# print $allruns['starttime'];
		$lastrun = $allruns;
		$allruns['numstarts'] = 1;
		$lastrun['PID'] = $pid;
		$lastrun['status'] = $status;
		return TRUE;
}

/* crawler_id is an int such that
   0 <= $crawler_id < $config_var['crawlserver']['Crawlers'] */
/* returns TRUE if successful or FALSE if not */
function crawler_command($crawler_run,$crawler_id,$command) {
	if (!readconfig($crawler_run, &$config_var, &$config_crawldir)) {
		# print "Read config...";
		return FALSE;
	}

	$cmdstring = "cd ".escapeshellarg($crawler_run)." && ./crawl_ctl.pl ".
		escapeshellarg($command)." ".escapeshellarg($crawler_id);
	$result = `$cmdstring`;
	# $result = `cd $crawler_run && ./crawl_ctl.pl $command $crawler_id`;
	# The ereg isn't quite right, but oh well.
	if (ereg('OK\r\n',$result)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

# Debug line longer displayed, because it could interfere with normal output:
# <!-- crawllib.php functions included / Wang Lam <wlam@cs.stanford.edu> -->
?>
