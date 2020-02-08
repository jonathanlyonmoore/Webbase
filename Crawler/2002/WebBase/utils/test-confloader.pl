#!/usr/bin/perl -w
#
# test-confloader.pl - lame test code for confloader.pl
# Wang Lam <wlam@cs.stanford.edu>
# January 2000
#
#

require 'confloader.pl';

&ConfLoader::loadValues($ARGV[0]);
shift @ARGV;

while(<>) {
	chomp;
	$answer = &ConfLoader::getValue($_);
	print "$_ = $answer\n";
}
