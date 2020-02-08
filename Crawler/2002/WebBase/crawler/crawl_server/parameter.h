/*
   The Stanford WebBase Project <webbase@db.stanford.edu>
   Copyright (C) 1999-2002 The Board of Trustees of the
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
#ifndef PARAMETER_H
#define PARAMETER_H

#include <fstream.h>
#include <string>
#include <vector>
#include <strstream.h>

#define SITESERVER "[siteserver]"
#define CRAWLSERVER "[crawlserver]"
#define DNSDBSERVER "[dnsdbserver]"
#define DNSUPDATER "[dnsupdater]"
#define STARTER "[starter]"
#define PARAMETER_FILE "crawl_config"

using namespace std;


template <class T>
inline void convert(const char* ptr, void* var)
{
    istrstream is(ptr);
    is >> *(T*)var;
}

template <class T>
inline void convertVec(const char* ptr, void* var)
{
  vector<T>* vecVar = (vector<T>*)var;
  
  T value;

  vector<string> strList;

  string strLine(ptr);

  split_string(strLine, ",", strList);

  if(strList.size() > 0) {
    vecVar->clear();
  }
    
  for(int i = 0 ; i < (int)strList.size() ; i++ ) {

    convert<T>(strList[i].c_str(), &value);

    vecVar->push_back(value);
  }
}

class Parameter {
  
 public:
  
  Parameter(const std::string strFileName);

  template <class T>
  void CreateRecord(T* var, const std::string strGroup, const std::string strTag, 
		    const std::string strOption,
		    const std::string strExplanation = "")
    {
      //if no option specified, it means this is not a command line option
      if(strOption != "") {

	config_entry record;
	
	record.strOption = strOption;
	record.strExplanation = strExplanation;
	record.pVar = (void*) var;
	record.fn = &convert<T>;  
	
	config_entry_list.push_back(record);
      }

      //if no tag is passed, it means entry doesn't exist in config file
      if(strTag != "") {
	string strValue;
	GetParameter(strGroup, strTag, strValue);
	convert<T>(strValue.c_str(), var);
	//	(*record.fn)(strValue.c_str(), var);
      }
    }

  template <class T>
  void CreateRecord(vector<T>* var, const std::string strGroup, const std::string strTag,
		    const std::string strOption,
		    const std::string strExplanation = "")
    {
      //if no option specified, it means this is not a command line option
      if(strOption != "") {

	config_entry record;
	
	record.strOption = strOption;
	record.strExplanation = strExplanation;
	record.pVar = (void*) var;
	record.fn = &convertVec<T>;  
	
	config_entry_list.push_back(record);
      }

      //if no tag is passed, it means entry doesn't exist in config file
      if(strTag != "") {
	string strValue;
	GetParameter(strGroup, strTag, strValue);
	convertVec<T>(strValue.c_str(), var);
	//	(*record.fn)(strValue.c_str(), var);
      }
    }
      
  void ProcessCommandLineOptions(const int argc, const char* argv[]);
      
  ~Parameter();
  
 private:
  typedef struct {
    std::string strOption;
    std::string strExplanation;
    void*  pVar;
    void (*fn)(const char*, void*);
    
  } config_entry;

  void GetParameter(const std::string strGroup, const std::string strTag, std::string& strValue);

  ifstream iFile;
  std::vector<config_entry> config_entry_list;
};

#endif

