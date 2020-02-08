<?php
	/* Session management */
	session_start();
	ob_start(); /* absorb any accidental output */
?>
<?php
	if (isset($HTTP_POST_VARS["crawler_run"])) {
		$_SESSION["crawler_run"]=$HTTP_POST_VARS["crawler_run"];
	}
?>
<?php
	error_reporting(E_ALL);
	require("crawllib.php");

	$file_status = FALSE;
	$shm_status = FALSE;
	$socket_status = TRUE;

	/* Get whatever command has been sent */
	$tail_lines = 20; $crawler_id = -1;
	if (isset($_SESSION["crawler_run"])) {
		$crawler_run=$_SESSION["crawler_run"];
	} else {
		/* If $crawler_run were initialized first, then
		   not only wouldn't it take the above =$_SESSION
		   assignment, but it would wipe the $_SESSION value too. */
		/* <shrug> */
		$crawler_run="";
	}
	if (isset($HTTP_POST_VARS["tail_lines"])) {
		$tail_lines = (int) $HTTP_POST_VARS["tail_lines"];
	}
	if (isset($HTTP_POST_VARS["crawler_id"])) {
		$crawler_id = (int) $HTTP_POST_VARS["crawler_id"];
	}

	/* We have to throw away a session write lock before we can run
	   commands that might not return (e.g., crawl_server). */
	session_write_close();

	if (($tail_lines > 0) && ($crawler_id >= 0) && 
		readconfig($crawler_run,&$config_var,&$config_crawldir) &&
		($crawler_id < $config_var['starter']['Crawlers'])) {

		$last_log = find_last_log($crawler_run.
			"/log/crawl_server.$crawler_id.");
		header("Content-type: text/plain");
		# print("/usr/bin/tail -$tail_lines $last_log\n");
		passthru("/usr/bin/tail -$tail_lines ".
			escapeshellarg($last_log));
	}

	ob_end_flush();
?>
<?php

/* Finds file named $prefix[0-9]+ where [0-9]+ is largest such number for
   which a file exists. */
function find_last_log($prefix)
{
	$i = 0;
	while (is_readable($prefix.sprintf("%03d",$i))) {
		++$i;
	}
	if ($i > 0) { --$i; }
	return $prefix.sprintf("%03d",$i);
}

?>
