--
-- Create tables for crawler operation
-- Wang Lam <wlam@cs.stanford.edu>
-- created Aug 2001
-- also see crawler/seed_list/load-aliases.pl
--

create table hosts (hid BIGINT PRIMARY KEY, hostname character varying(1024) unique not null, priority real not null, minpages integer, maxpages integer not null, mindepth integer,  maxdepth integer not null, imagesp boolean, pdfsp boolean, audiop boolean, minpause integer not null, rooturls text);

-- all tables should have hid -> hosts.hid referential integrity, and
-- on delete cascade.
-- host.hid should not be deletable if host is being crawled.

create table aliases (hid bigint not null, alias character varying (1024) not null);
create index aliases_hid_skey on aliases (hid);

create table ipaddrs(hid bigint not null, ipaddr inet not null);
create index ipaddrs_hid_skey on ipaddrs (hid);

-- Deprecated (crawling is in hid order, not priority order):
--create index hosts_priority_skey on hosts(priority);

-- to manually create and fill tables:
create table uncrawled (hid BIGINT PRIMARY KEY, ipaddr inet NOT NULL);
create table crawling (hid BIGINT PRIMARY KEY, id integer NOT NULL, ipaddr inet NOT NULL);
create index crawling_ipaddr_skey on crawling(ipaddr);
create table crawled (hid bigint primary key, id integer, fetched integer, status varchar(1024));

-- to populate from table hosts:
--create table uncrawled (hid, ipaddr) as (select h.hid, (select ipaddr from ipaddrs where hid = h.hid order by ipaddr limit 1) from hosts h);
--create index uncrawled_hid_skey on uncrawled(hid);
--create table crawling (hid BIGINT PRIMARY KEY, id integer NOT NULL, ipaddr inet NOT NULL);
--create index crawling_ipaddr_skey on crawling(ipaddr);
--create table crawled (hid bigint primary key, id integer, fetched integer, status varchar(1024));

-- PostgreSQL only:
vacuum analyze

