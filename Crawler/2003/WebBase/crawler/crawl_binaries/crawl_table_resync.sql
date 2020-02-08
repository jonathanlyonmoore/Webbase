--
-- Move sites in crawling back to uncrawled, to allow dead crawls to resume
-- Wang Lam <wlam@cs.stanford.edu>
-- created Nov 2002
-- also see crawler/seed_list/load-aliases.pl
--
-- This file is now obsoleted by crawl_unroll.pl, which provides a
-- finer-grained version of the same operation.
--

-- Copy crawling to uncrawled
insert into uncrawled select hid,ipaddr from crawling;

-- Empty crawling
delete from crawling;
-- PostgreSQL also allows
-- truncate crawling;

-- PostgreSQL only:
vacuum analyze

