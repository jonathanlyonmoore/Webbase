/*
   The Stanford WebBase Project <webbase@db.stanford.edu>
   Copyright (C) 1999-2003 The Board of Trustees of the
   Leland Stanford Junior University
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifndef REPOSITORY_H
#define REPOSITORY_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "PageContainer.h"
#include "SiteContainerMap.h"
#include "PathGenerator.h"
#include <vector>
#include <string>
// #include <hash_map>
#include <ctime>
#include <inttypes.h>

// FIXME A temporary typedef, to make int64 -> int64_t less painful.
typedef int64_t int64;
typedef uint64_t uint64;

/** Interface to access a repository of Web pages. */
/** A repository provides ordered access to a large pool of Web resources,
    for streaming, and random access within each Web site. The repository is 
    managed as a set of page containers. Each container contains the Web
    resources belonging to a specific site. The definition of a "site" and the 
    canonical  name for each site are as prescribed in the crawler.
    Many member functions return a negative error code on failure; for
    error codes, see the constants in FSBigFile.h.
    \see FSBigFile
*/
class repository {
   public:
      /** Constructor/open a filesystem repository.
          \param repName the name of the repository.
                 NOTE: This parameter is currently not used by the filesystem-based
                 implementation of the repository.
          \param compressed true if the contents should be compressed with zlib.
          \param rootDir the root of the repository on the file system
          \param mode open in read-only mode if 'r', write mode otherwise.
      */
      repository(const std::string repName, bool compressed = true, 
                 const std::string rootDir = std::string(), char mode = 'r');

#ifdef HAVE_SRB
      /** This class must be compiled with SDSC's Storage Resource Broker
          when this section is to be used. */

      /** Constructor/open a SRB repository.
          \param repName name of the SRB collection corresponding to this
                         repository
          \param bool compressed true if the contents should be compressed
          \param mode open in read-only mode if 'r', write mode otherwise
      */
      repository(const std::string repName, bool compressed = true, char mode = 'r');
#endif

      /** Destructor/close the repository. */
      ~repository();

      /** Appends a resource to repository. */
      /** When no more stores for a site are forseen, an end_site call should
          follow the last store call for the site. This method can be called
          only if the repository has been opened in "write" ('w') mode.

          \param site canonical hostname of the resource's Web site.
                      This parameter is used as an opaque string identifying
		                  a Web site in some unique way.
          \param url  the URL of the resource, as fetched, no longer than
                      MAX_URL_SIZE.
          \param buf  the resource itself, as a sequence of bytes.
	        \param size sizeof buf, not to exceed MAX_FILE_SIZE.
	        \see MAX_URL_SIZE
      	  \see MAX_FILE_SIZE
          \see end_site()
          \return zero if successful, a negative error code if failed. */
      int store(const std::string site, const std::string url, 
                const char *buf, int size);

      /** Provides hint to repository that stores for a site are finished. */
      /** This call indicates to the repository that stores for a particular
          site are finished, and that no more are coming.  The call is only
          advisory, so that the repository may release resources it reserved 
          for the site.  In particular, the call does not actually guarantee 
          that there will not be more stores in the future, and the repository 
          is not obliged to act on this call or prevent future stores to
          the site.

          That is, this call does not guarantee anything, but should be called
          anyway when no more stores for a site are forseen to conserve
          repository resources.

          \param site canonical hostname of the resource's Web site.
                      This parameter is used as an opaque string identifying
		      a Web site in some unique way, and should match a
		      site that was used for calls to store.
          \param numpages estimated number of pages stored in site.
                      This is hopefully the number of distinct urls
                      for site for which store() has been called.
                      This number could be 0, if no store() calls occurred,
                      or -1 if the number is unknown.
          \see store() */
      void end_site(const std::string site, int numpages = -1);

      /** Reads the next resource in the repository. */
      /** The repository currently does not compress on-the-fly, so uncompr 
          should not be false unless the repository was stored compressed. This 
          method can be called only if the repository has been opened in read mode.
           
          \param url  replaced with the URL of the resource being returned,
                      not longer than MAX_URL_SIZE.
          \param time replaced with the time when the resource was saved.
          \param buf  filled with the resource itself, as a sequence of bytes
                      no larger than size.  What happens if size is
            		      too small for the resource?
          \param size sizeof buf, replaced with the size of the resource.
                      Neither value should exceed MAX_FILE_SIZE.
          \see MAX_URL_SIZE
          \see MAX_FILE_SIZE
          \param uncompr store uncompressed resource in buf if true, compressed
                         resource in buf if false.
          \return zero if successful, a negative error code if failed. */
      int read(std::string& url, std::time_t& time, char* buf, int& size, 
               bool uncompr = true);
      
      /** Sets the current Web site in the repository for future reads. */
      /** This method can be called only when the repository has been opened in read mode. */
      /** \param site canonical hostname of the Web site.
                      This parameter is used as an opaque string identifying
		                  a Web site in some unique way, and should match whatever
            		      site was used to store the desired resource(s).
          \return zero if successful, a negative error code if failed. */
      int set_site(std::string site);

      /** Get the canonical name of Web site being read. */
      /** \returns name of current Web site, suitable for use in setdomain,
                   or std::string("") if there is no site to read.
          \see set_site() */
      std::string get_site() const;

      /** Seeks to a byte offset in the crawl of the current Web site. */
      /** \param offset offset into the crawl of the current site.
          \param whence SEEK_SET, SEEK_CUR, or SEEK_END, as in seek(3).
          \returns zero if successful, a negative error code if failed. */
      int seek(int64_t offset, int whence);

      /** Gets the current byte offset in the crawl of the current Web site. */
      /** \returns the offset into the current site, suitable for use in seek.
                   a negative value indicates error.
          \see seek() */
      int64_t tell() const;

 private:
      static const int FS_STORE = 1;      /**< use filesystem with 2GB limits */
      static const int SRB_STORE = 2;     /**< use SDSC's Storage Resource Broker */

      std::string repName;                   /* name of the repository */
      bool compressed;                       /* if true, resources will be compressed */
      char mode;                             /* mode in which repository is opened */
      std::string current_site;              /* current site being read/streamed out */
      PageContainer *current_container;      /* the container corres. to current_site */
      SiteContainerMap scMap;                /* a map from a site to its crres. containerInfo object */
      PathGenerator pathGen;                 /* generates paths in which to create containers */
      int storeType;                         /* Type of underlying storage being used */


      PageContainer *openContainerForSite(std::string site, char mode);
};

#endif /* REPOSITORY_H */
