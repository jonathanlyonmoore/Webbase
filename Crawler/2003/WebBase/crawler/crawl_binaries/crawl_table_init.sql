--
-- Reload crawler-state tables from Hosts table, deleting any prior state
-- Wang Lam <wlam@cs.stanford.edu>
-- created Aug 2001, last updated Apr 2003
-- also see crawler/seed_list/load-aliases.pl
--

-- DELETE FROM uncrawled;
-- DELETE FROM crawling;
-- DELETE FROM crawled;
-- PostgreSQL only:
TRUNCATE uncrawled;
TRUNCATE crawling;
TRUNCATE crawled;

-- to populate from table hosts:
INSERT INTO uncrawled
   (SELECT h.hid, (SELECT ipaddr FROM ipaddrs WHERE hid = h.hid 
                   ORDER BY ipaddr LIMIT 1)
    FROM Hosts h);

-- PostgreSQL only:
VACUUM ANALYZE

