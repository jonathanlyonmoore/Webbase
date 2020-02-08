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
#ifndef __SITE_CONTAINER_MAP_H_
#define __SITE_CONTAINER_MAP_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/* #include <map> */
#include <string>
#include "PageContainer.h"

/** Some function objects and typedefs to define a map from
  * string to PageContainer*.
  *
  * Sriram Raghavan <rsram@cs.stanford.edu>
  */

struct hashString
{
  static hash<const char *> h;
  size_t operator()(const string& s) const {
    return h(s.c_str());
  }
};

struct ContainerInfo
{
  PageContainer *pcPtr;
  int pageCount;

  ContainerInfo() { pcPtr = NULL; pageCount = 0; }
  ContainerInfo(PageContainer *pPtr, int pCount=0) { pcPtr = pPtr; pageCount = pCount; }
};

typedef hash_map<std::string, ContainerInfo, hashString> SiteContainerMap;
/* typedef std::map<std::string, PageContainer *> SiteContainerMap; */
typedef SiteContainerMap::iterator SiteContainerIter;

#endif
