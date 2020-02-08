<?php
	/* Session management */
	session_start();
	ob_start(); /* absorb any accidental output */

	/* For Location:, HTTP/1.1 requires absolute URI. */
	$next_link = "http://" . $HTTP_SERVER_VARS['HTTP_HOST'];
	if (dirname($HTTP_SERVER_VARS['PHP_SELF']) != '/') {
		$next_link .= dirname($HTTP_SERVER_VARS['PHP_SELF']);
	}
	$next_link .= "/crawlstat?".SID;
	$next_link = "$next_link";  /* Saves next_link when SID goes away */
?>
<?php
	/* If form changes our session state, apply them. */
	if (isset($HTTP_POST_VARS["css"])) {
		$_SESSION["css"]=$HTTP_POST_VARS["css"];
	}
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
	$cmd = ""; $crawler_id = -1;
	if (isset($_SESSION["crawler_run"])) {
		$crawler_run=$_SESSION["crawler_run"];
	} else {
		/* If $crawler_run were initialized first, then
		   not only wouldn't it take the above =$_SESSION
		   assignment, but it would wipe the $_SESSION value too. */
		/* <shrug> */
		$crawler_run="";
	}
	if (isset($HTTP_POST_VARS["cmd"])) {
		$cmd = $HTTP_POST_VARS["cmd"];
	}
	if (isset($HTTP_POST_VARS["crawler_id"])) {
		$crawler_id = (int) $HTTP_POST_VARS["crawler_id"];
	}

	/* We have to throw away a session write lock before we can run
	   commands that might not return (e.g., crawl_server). */
	session_write_close();

	if ($cmd && ($crawler_id >= 0) && 
		readconfig($crawler_run,&$config_var,&$config_crawldir) &&
		($crawler_id < $config_var['starter']['Crawlers'])) {
		$cmd = strtolower($cmd);
		switch ($cmd) {
			case 'pause':
			case 'resume':
			case 'start':
			case 'end':
			/* We ignore 'status', since we already do it */
				crawler_command($crawler_run,$crawler_id,$cmd);
			default:
			/* Invalid command: ignore */
		}
	}

	/* Redirect to the idempotent crawlstat.php. */
	header("Location: $next_link");
	ob_end_flush();
	/* Does this URI need data to accompany status 302? */
?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN"
        "http://www.w3.org/TR/html4/strict.dtd">
<HTML>
<HEAD>
<TITLE>WebBase Crawler Status - redirect failed</TITLE>
<LINK href="<?php echo $css ?>" rel="stylesheet" type="text/css">
</HEAD>
<BODY>

<H1>Redirect Failed</H1>

<P>
Your Web browser should have followed an HTTP directive to return to the
<A href="<?php echo $next_link ?>">status page</A> after your command.
Failure to do so indicates an error in the Web crawler monitor, 
in your Web browser, or on a network connection.
</P>
<P>
Please use the <A href="<?php echo $next_link ?>">status page link</A> to
return to the status page.
</P>

<HR>
<ADDRESS>
crawlstat/crawlcmd/crawllib - Wang Lam &lt;wlam@cs.stanford.edu&gt; March 2002
</ADDRESS>
</BODY>
</HTML>

