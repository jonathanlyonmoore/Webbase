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

/** Implementations for the methods in the repository class
  *
  * Sriram Raghavan <rsram@cs.stanford.edu>
  */

#include "repository.h"
#include "FSPageContainer.h"
#include "PathGenerator.h"
// #include <map>
#include <vector>
#include <string>
#include <ctime>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

repository::repository(const std::string repName, bool compressed, const std::string rootDir, 
                       char mode)
{
  this->repName = repName;
  this->compressed = compressed;
  this->mode = mode;
  pathGen.setPathPrefix(rootDir);
  current_site = ""; 
  current_container = NULL;
  storeType = FS_STORE;

  cerr << "Repository: Initialized with storeType = FS_STORE, rootDir = " << rootDir 
       << ", mode = " << mode << ", compressed = " << compressed << endl;
}

#ifdef HAVE_SRB
repository::repository(const std::string repName, bool compressed = true, char mode = 'r')
{
  this->repName = repName;
  this->compressed = compressed;
  this->mode = mode;
  storeType = SRB_STORE;

  // Do SRB-specific initialization here

  cerr << "Repository: Initialized with storeType = SRB_STORE< rootDir = " << rootDir 
       << ", mode = " << mode << ", compressed = " << compressed << endl;
}
#endif


repository::~repository()
{
  // Delete container currently opened for read, if any
  if (current_container != NULL) delete current_container;

  // Delete all containers that may be opened for write at this time
  // All such containers will be present in scMap
  SiteContainerIter scIter;
  for (scIter = scMap.begin(); scIter != scMap.end(); scIter++)
    delete (PageContainer *)(scIter->second.pcPtr);

  cerr << "Repository: End of destructor. Done............." << endl;
}


PageContainer* repository::openContainerForSite(std::string site, char mode)
{
  PageContainer *siteContainer = NULL;


  cerr << "Repository: Opening container for site " << site 
       << " in '" << mode << "' mode" << endl;
  
  // Open the page container for the new site. Two cases: filesystem or SRB
  if (storeType == FS_STORE) {

    // Compute the container path corres. to this site
    std::vector<std::string> dirList;
    std::string dir = pathGen.getPathFor(site, TypeDetector::TEXT);
    dirList.push_back(dir);
    
    // Make sure the path (dir) is created and usable, so that FSPageContainer
    // won't panic when it stats the path for directoryness.  --wlam
    // P.S. Please check code for correctness, and remove this postscript if
    // verified.  My copy of Stroustrup is not at the office...
    if(mode == 'w') { // TH: only create dirs if write mode ??!!
      std::string::size_type slash = 0;
      std::string partialDir;
      do {
	slash = dir.find('/',slash+1);
	if (slash == std::string::npos) slash = dir.size();
	partialDir.assign(dir,0,slash);
	//      cerr << "Repository: checking path " << partialDir << endl;
	
	struct stat buf;
	if (stat(partialDir.c_str(),&buf) == -1) {
	  if (mkdir(partialDir.c_str(),0777) != 0) {   // Let umask sort out permissions
	    cerr << "Repository: cannot create directory " << partialDir << '!' << endl;
	    return NULL;
	  }
	} else {
	  if (!(buf.st_mode & S_IFDIR)) {
	    cerr << "Repository: " << partialDir << " must be, but is not,"
		 <<	"a directory!  Cannot open container for site " << site << endl;
	    return NULL;
	  }
	}
      } while (slash < dir.size());
      
      cerr << "Repository: Verified container path " << dir << endl;
    } else if(mode == 'r') {
      struct stat buf;
	
      if (lstat(dir.c_str(), &buf) < 0) {
	return NULL;
      }
      if (!S_ISDIR(buf.st_mode)) {
	return NULL;
      }

      cerr << "Repository: Verified container path " << dir << endl;
    }

    // First parameter is the full path and name of the chunklist file for this
    // sub-repository
    // NOTE: We are passing pointer to automatic variable dirList because
    // FSBigFile makes a local copy of the vector.
    siteContainer = new FSPageContainer(dir + "/repository", compressed, &dirList, mode);
  }

//   else
//     siteContainer = new SRBPageContainer(....);


  cerr << "Repository: Successfully opened container for " << site << endl;
  
  return siteContainer;
}


int repository::store(const std::string site, const std::string url, const char *buf, int size)
{
  cerr << "Repository: Attempting to store " << url 
       << " in container for site " << site << endl;

  // Cannot store to a repository opened in read-mode
  if (mode == 'r') {
    cerr << "Repository: Cannot write to a repository opened in read mode" << endl;
    return -1;
  }

  SiteContainerIter scI = scMap.find(site);
  int returnCode = 0;

  // Container for this site is already opened and ready to append to
  if (scI != scMap.end()) {
    PageContainer *targetContainer = (PageContainer *)(scI->second.pcPtr);
    returnCode = targetContainer->storePage(url, buf, size);
    if (returnCode >= 0) scI->second.pageCount++;
  }

  else {
    // New site. Open a new container for this site and add to the hash map. Finally, attempt
    // to store the page in the new container
    PageContainer *newContainer = openContainerForSite(site, 'w');
    if (newContainer) {
      ContainerInfo cInfo(newContainer, 0);
      returnCode = newContainer->storePage(url, buf, size);
      if (returnCode >= 0) cInfo.pageCount = 1;
      scMap[site] = cInfo;
    }
    else returnCode = FSPageContainer::BIGFILE_ERROR;
  }
  return returnCode;
}


void repository::end_site(const std::string site, int numpages)
{
  // Locate container corres. to this site, delete it (which will also result in
  // the underlying bigfile being closed), and remove from the hash_map

  cerr << "Repository: Crawler reported end of site " << site << endl;

  SiteContainerIter scIter = scMap.find(site);
  if (scIter != scMap.end()) {
    PageContainer *containerToClose = (PageContainer *)(scIter->second.pcPtr);
    ostringstream numpagesStream;  

    // Store the page count as given by crawler
    numpagesStream << numpages;
    containerToClose->addProperty("CRAWLER_NUMPAGES", numpagesStream.str());

    // store page count from the repository's point of view
    numpagesStream.str("");
    numpagesStream << scIter->second.pageCount;
    containerToClose->addProperty("REPOSITORY_NUMPAGES", numpagesStream.str());

    delete containerToClose;
    scMap.erase(scIter);
    cerr << "Repository: Closed the container for site  " << site << endl;
  }
  else {
    cerr << "Repository: end_site was called for a site that was not open in the first place!" << endl;
    cerr << "Repository: continuing anyway..." << endl;
  }
}


int repository::read(std::string& url, std::time_t& time, char* buf, int& size, bool uncompr)
{
  // Verify that repository has been opened in read mode
  if (mode != 'r') {
    cerr << "Repository: Cannot read from a repository opened in write mode" << endl;
    return -1;
  }

  // Check for initialized current_container
  if (current_container == NULL) {
    cerr << "Repository: Call set_site first, before reading" << endl;
    return -2;
  }

  // Read from the current container
  return current_container->readPage(url, time, buf, size, uncompr);
}


int repository::set_site(std::string site)
{
  // Verify that repository has been opened in read mode
  if (mode != 'r') {
    cerr << "Repository: set_site can only be called when repository is opened in read mode" << endl;
    return -1;
  }

  // Delete the current container
  delete current_container;
  current_container = NULL;

  // Open container corres. to new site in read mode
  current_container = openContainerForSite(site, 'r');

  if(current_container == NULL) {
    cerr << "Repository: could not set site to " << site << endl;
    cerr << "skipping." << endl;
    return -1;
  }

  cerr << "Repository: Current site set to " << site << endl;

  return 1;
}


std::string repository::get_site() const
{
  return current_site;
}


int repository::seek(int64_t offset, int whence)
{
  cerr << "Repository: Seeking to offset " << offset 
       << " in container for site " << current_site << endl;

  if (current_container != NULL) return current_container->seek(offset, whence);
  cerr << "Repository: There is no currently open site container in which to seek" << endl;
  return -1;
}

int64_t repository::tell() const
{
  if (current_container != NULL) return current_container->tell();
  cerr << "Repository: There is no currently open site container on which to call tell" << endl;
  return -1;
}
