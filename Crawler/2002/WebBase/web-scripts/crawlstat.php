<?php
/* crawlstat.php - crawler status display and control interface
   Wang Lam <wlam@cs.stanford.edu> March 2002
*/
	/* Session management */
	session_start();

	if (!isset($_SESSION["css"])) {
		$_SESSION["css"]="default.css";
	}
	$css = $_SESSION["css"];

	if (!isset($_SESSION["crawler_run"])) {
		$_SESSION["crawler_run"]="/tmp/wlam/crawler/crawl_binaries";
	}
	$crawler_run = $_SESSION["crawler_run"];
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"
	"http://www.w3.org/TR/html4/strict.dtd">
<HTML>
<HEAD>
<TITLE>WebBase Crawler Status - <?php echo $crawler_run ?></TITLE>
<LINK href="<?php echo $css ?>" rel="stylesheet" type="text/css">
</HEAD>
<BODY>
<H1>WebBase Crawler Status</H1>

<?php
	error_reporting(E_ALL);
	require("crawllib.php");

	$file_status = FALSE;
	$shm_status = FALSE;
	$socket_status = TRUE;

	/* The crawl_binaries path defines an instance of a WebBase crawler. */
	# if (preg_match('/(\d+)/',$_SERVER["QUERY_STRING"],$subdir)) {
		# $crawler_run="/dfs/pita/0/tmp/$subdir[1]/dli2/src/WebBase/crawler/crawl_binaries";
	# } else {
	#	$crawler_run="/dfs/hake/7/wlam/dli2/src/WebBase/crawler/crawl_binaries";
		# $crawler_run="/tmp/wlam/crawler/crawl_binaries";
	# }
	if (!readconfig($crawler_run,&$config_var,&$config_crawldir)) {
?>
<P>
Error: Cannot read configuration in <?php echo $crawler_run ?>.
</P>
<FORM action="crawlcmd?<?php echo SID ?>" method="POST">
 <P>
 <LABEL>Please specify a new crawl directory
  <INPUT type="text" name="crawler_run" value="<?php echo $crawler_run ?>">
 </LABEL>
 <INPUT type="submit" value="Change directory">
 </P>
</FORM>
<?php
		footer();
		return;
	}

	$displayconfig = TRUE;
	$displayallruns = FALSE;
	$displaycurrentrun = TRUE;
?>

<P>
Crawler directory: <?php print $crawler_run; ?>
</P>
<FORM action="crawlcmd?<?php echo SID ?>" method="POST">
 <P>
 <LABEL>Change crawl directory to
  <INPUT type="text" name="crawler_run" value="<?php echo $crawler_run ?>">
 </LABEL>
 <INPUT type="submit" value="Change directory">
 </P>
</FORM>

<?php
	if ($displayconfig) {
?>
<H2>Configuration information</H2>

<TABLE rules="all" border="1">
<TR>
 <TH>Section</TH>
 <TH>Parameter</TH>
 <TH>Value</TH>
</TR>
<?php
	foreach ($config_var as $section => $assignments) {
		$firstline = TRUE;
?>
<TR>
 <TD><?php print "$section" ?></TD>
<?php
		foreach ($assignments as $param => $value) {
			if (! $firstline) {
?>
<TR>
  <TD></TD>
<?php
			}
?>
  <TD><?php print "$param" ?></TD>
  <TD><?php print "$value" ?></TD>
</TR>
<?php
			$firstline = FALSE;
		}
	}
?>
</TABLE>

<H2>Crawler-process output directories</H2>
<UL>
<?php
	for ($i = 0; $i < $config_var['starter']['Crawlers']; ++$i) {
		print "<LI>Crawler $i - $config_crawldir[$i]\n";
	}
?>
</UL>

<H2>Crawler-process log files</H2>
<UL>
<?php
	for ($i = 0; $i < $config_var['starter']['Crawlers']; ++$i) {
		print "<LI>Crawler $i - $crawler_run/log/crawl_server.$i.*\n";
?>
 <FORM action="crawllog?<?php echo SID ?>" method="POST">
  <P>
  <INPUT type="hidden" name="crawler_run" value="<?php echo $crawler_run ?>">
  <INPUT type="hidden" name="crawler_id" value="<?php echo $i ?>">
  <INPUT type="submit" value="Display newest entries in log">
  <LABEL>
   of up to <INPUT type="text" name="tail_lines" value="20"> lines.
  </LABEL>
  </P>
 </FORM>
<?php
	}
?>
</UL>
<?php
	}
?>

<?php
	/* Check out the running crawlers */
	$allruns = Array(); $lastrun = Array(); $fail = Array();
	for ($i = 0; $i < $config_var['starter']['Crawlers']; ++$i) {
		$ret = FALSE;
		if ($shm_status || $socket_status) {
			$ret=statcrawler($crawler_run,$i,&$allruns[$i],&$lastrun[$i]);
			# print $allruns[$i]['PID'];
		} else if ($file_status) {
			$ret=parselog($crawler_run,$i,&$allruns[$i],&$lastrun[$i]);
		} else {
			$ret=FALSE; /* No status */
		}
		$fail[$i] = !$ret;
	}
?>

<?php
	if ((!$shm_status) && $displayallruns) {
	# if ($displayallruns) {
?>
<H2>Crawler-process statistics (over all invocations)</H2>

<TABLE>
<TR>
 <TH>Crawler ID</TH>
 <TH>Number of invocations</TH>
 <TH>Number of bytes</TH>
 <TH>Number of pages</TH>
 <TH>Time run</TH>
 <TH>Average throughput (bytes/sec)</TH>
</TR>
<?php
	for ($i = 0; $i < $config_var['starter']['Crawlers']; ++$i) {
		if (! $fail[$i]) {
			$totaltime = $allruns[$i]["endtime"]-
				$allruns[$i]["starttime"];
			$throughput = $totaltime ?
				$allruns[$i]["numbytes"] / $totaltime :
				0;
			$throughput = sprintf("%d",$throughput);
?>
<TR>
 <TD><?php print $i ?></TD>
 <TD><?php print $allruns[$i]["numstarts"] ?></TD>
 <TD><?php print $allruns[$i]["numbytes"] ?></TD>
 <TD><?php print $allruns[$i]["numpages"] ?></TD>
 <TD><?php print $totaltime ?> secs</TD>
 <TD><?php print $throughput ?></TD>
</TR>
<?php
# EOF;
		 } else {
			# print <<<EOF
?>
<TR>
 <TD><?php print $i ?></TD>
 <TD colspan="5">No information available.</TD>
</TR>
<?php
# EOF;
		}
	}
?>
</TABLE>
<?php
	}
?>


<?php
	if ($displaycurrentrun) {
?>
<?php	if ($shm_status) { ?>
<H2>Crawler-process statistics (current invocation)</H2>
<?php	} else { ?>
<H2>Crawler-process statistics (last invocation)</H2>
<?php	} ?>

<TABLE>
<TR>
 <TH>Crawler ID</TH>
 <TH>Crawler PID</TH>
 <TH>Bytes</TH>
 <TH>Pages</TH>
 <TH>Time run</TH>
 <TH>Effective throughput (bytes/sec)</TH>
 <TH colspan="3">Status</TH>
</TR>
<?php
/* We have some possible buttons:
	status     useful buttons
	-------------------------
	running:   pause kill
	paused:    resume kill
	dead:      start
   Unfortunately, a late-started process sits idle... will it get
   new requests without ./starter?
*/
	$totalbytes = 0; $totalpages = 0; $totalthroughput = 0;
	$totalrunning = 0; $totalcrawling = 0; $totalpaused = 0;
	for ($i = 0; $i < $config_var['starter']['Crawlers']; ++$i) {
		if (! $fail[$i] ) {
			$totaltime = $lastrun[$i]["endtime"]-
				$lastrun[$i]["starttime"];
			$throughput = $totaltime ?
				$lastrun[$i]["numbytes"] / $totaltime :
				0;
			$throughput = sprintf("%d",$throughput);
?>
<TR>
 <TD><?php print $i ?></TD>
 <TD><?php print $lastrun[$i]["PID"] ?></TD>
 <TD><?php print $lastrun[$i]["numbytes"] ?></TD>
 <TD><?php print $lastrun[$i]["numpages"] ?></TD>
 <TD><?php print $totaltime ?> secs</TD>
 <TD><?php print $throughput ?></TD>
<?php
			$totalbytes += $lastrun[$i]["numbytes"];
			$totalpages += $lastrun[$i]["numpages"];
			$totalthroughput += $throughput;
			++$totalrunning;
			if ($lastrun[$i]["status"] == "paused") {
				++$totalpaused;
			} else {
				++$totalcrawling;
			}
			$label = ($lastrun[$i]["status"] == "paused") ?
				 "Resume" : "Pause";
?>
 <TD><?php echo $lastrun[$i]["status"] ?></TD>
 <TD>
  <FORM action="crawlcmd?<?php echo SID ?>" method="POST">
   <P>
   <INPUT type="hidden" name="crawler_id" value="<?php echo $i ?>">
   <INPUT type="hidden" name="cmd" value="<?php echo $label ?>">
   <INPUT type="submit" value="<?php print $label ?>">
   </P>
  </FORM>
 </TD>
 <TD>
  <FORM action="crawlcmd?<?php echo SID ?>" method="POST">
   <P>
   <INPUT type="hidden" name="crawler_id" value="<?php echo $i ?>">
   <INPUT type="hidden" name="cmd" value="end">
   <INPUT type="submit" value="Kill crawler">
   </P>
  </FORM>
 </TD>
</TR>
<?php		 } else { ?>
<TR>
 <TD><?php print $i ?></TD>
 <TD colspan="5">
<?php
			if ($shm_status || $socket_status) {
				print "Running crawler not found\n";
			} else {
				print "Error: No information available.";
			}
?>
 </TD>
 <TD>
  <FORM action="crawlcmd?<?php echo SID ?>" method="POST">
   <P>
   <INPUT type="hidden" name="crawler_id" value="<?php echo $i ?>">
   <INPUT type="hidden" name="cmd" value="start">
   <INPUT type="submit" value="New crawler">
   </P>
  </FORM>
 </TD>
 <TD colspan="2">no running crawler</TD>
</TR>
<?php
		}
	}
?>
<TR>
 <TH colspan="2">Totals for <?php echo $totalrunning ?> crawlers:</TH>
 <TD><?php echo $totalbytes ?></TD>
 <TD><?php echo $totalpages ?></TD>
 <TD>N/A</TD>
 <TD><?php echo $totalthroughput?></TD>
 <TD><?php echo $totalcrawling ?> running</TD>
 <TD><?php echo $totalpaused ?> paused</TD>
 <TD><?php echo
	$config_var['starter']['Crawlers']-$totalrunning ?> unavailable</TD>
</TR>
</TABLE>
<?php
	}
	footer();
?>
<?php
function footer() {
	global $css;
?>

<HR>

<H2>Display formatting</H2>

<FORM action="crawlcmd?<?php echo SID ?>" method="POST">
 <P>
 <LABEL>Current style sheet
  <INPUT type="text" name="css" value="<?php echo $css ?>">
 </LABEL>
 <INPUT type="submit" value="Change style sheet">
 </P>
</FORM>

<HR>
<ADDRESS>
crawlstat/crawlcmd/crawllib - Wang Lam &lt;wlam@cs.stanford.edu&gt; March 2002
</ADDRESS>
</BODY>
</HTML>
<?php
}
?>

