#!/usr/bin/perl
#
# Simple cgi wrapper for getting the cached webbase pages
#
# Author: Taher H. Haveliwala (taherh@cs.stanford.edu)
#

use warnings;
use strict;

#use lib "/u/taherh/perl/modules/";
use lib "/u/taherh/research/latest/perllib/";

use CGI;
use Socket;
use IO::Socket;
use FileHandle;
use URI::Escape;
use IPC::Open2;

use WB::Properties;

STDOUT->autoflush();

print "Content-type: text/html\r\n\r\n";

my $q = new CGI;
my $url = $q->param('url');

$url = "http://$url" unless $url =~ /^http:\/\//;

print "<HTML>\n";
print "<HEAD>
         <TITLE>Cached Document</TITLE>
         <base href='$url'>
       </HEAD>\n";
print "<BODY>\n";
print "<h1 align=center>Cache of: <a href='$url'>$url</a></h1>\n";
print "<h3 align=center>(Any inline images are not cached copies; they are current)</h3>\n";
print WB::Properties::get_content($url);
print "</body></html>\n";
