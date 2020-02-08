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
#include <iostream.h>
#include <getopt.h>
#include <stdio.h>
#include "parameter.h"
#include "my_utility.h"

using namespace std;


void Parameter::ProcessCommandLineOptions(const int argc, const char* argv[])
{
  struct option *long_options;
  int    num_options = 0;

  if (config_entry_list.size() == 0) return;

  long_options = new struct option[config_entry_list.size()+1];
  for (unsigned i = 0; i < config_entry_list.size(); i++) {
    if (config_entry_list[i].strOption.size() != 0) {
      long_options[num_options].name = config_entry_list[i].strOption.c_str();
      long_options[num_options].has_arg = 1;
      long_options[num_options].flag = NULL;
      long_options[num_options].val = 1000 + i;
      num_options++;
    }
  }
  long_options[num_options].name = NULL;
  long_options[num_options].has_arg = 0;
  long_options[num_options].flag = NULL;
  long_options[num_options].val = 0;

  while (1) {
    int option_index = 0;
    int c = getopt_long_only(argc, (char *const *)argv, "",
			     long_options, &option_index);
    if (c == -1) break;
    if (1000 <= c && c < 1000 + num_options) {
      config_entry_list[option_index].fn
	(optarg, config_entry_list[option_index].pVar);
    } else {
      fprintf(stderr, "Usage: %s [options]\n", argv[0]);
      for (unsigned i = 0; i < config_entry_list.size(); i++) {
	if (config_entry_list[i].strExplanation.size() != 0) {
	  fprintf(stderr, "\t-%s %s\n", 
		  config_entry_list[i].strOption.c_str(),
		  config_entry_list[i].strExplanation.c_str());
	}
      }
      exit(1);
    }
  }
  
  delete [] long_options;
}

Parameter::Parameter(const string strFileName) {
  iFile.open(strFileName.c_str());

  if(! iFile) {
    cout << "Parameter::Could not open file: " << strFileName << endl;
  }
}

void Parameter::GetParameter(const string strGroup, const string strTag, string& strValue) {
  string strLine;

  //set file pointer to beginning of file
  iFile.seekg(0);

  //read a line from file till Group is found, or end of file is reached
  do {
    getline(iFile, strLine);
  
  } while(strLine != strGroup && iFile);
  
  //if end of file reached...
  if(!iFile) {
    cout << "Parameter::Could not find Group: " << strGroup << endl;
    iFile.clear();
    return;
  }

  //search the parameter in file
  string strFirst, strLast;
  int pos;
  do {
    getline(iFile, strLine);
    if(!iFile) break;
    if(strLine.size() == 0) continue;

    pos = strLine.find("=", 0);

    strFirst = strLine.substr(0, pos);
    strLast = strLine.substr(pos + 1, strLine.length() - pos - 1);

  } while(strFirst != strTag);

  //if parameter not found...
  if(!iFile) {
    cout << "Parameter::Could not find parameter: " << strTag << endl;
    iFile.clear();
    return;
  }

  //get the actual parameter value
  strValue = strLast;
}

Parameter::~Parameter() {

  iFile.close();
}

/*
void main(int argc, const char* argv[]) {
  Parameter param("crawl_config");

  short crawlers;
  param.CreateRecord(&crawlers, "[starter]", "crawlers", "c", "number of crawlers");

  cout << "crawlers: " << crawlers << endl;

  string logfile;
  param.CreateRecord(&logfile, "[siteserver]", "logfile", "l", 
		     "log file of site server");

  cout << "Log file of site server: " << logfile << endl;  
  
  vector<string> crawlerportList;
  param.CreateRecord(&crawlerportList, "[crawlserver]", "localport", "l", "port of crawler");

  for(int i = 0 ; i < (int) crawlerportList.size() ; i ++ ) { 
    cout << "port of crawler: " << crawlerportList[i] << endl;
  }
  param.ProcessCommandLineOptions(argc, argv);
  cout << "crawlers: " << crawlers << endl;
  cout << "Log file of site server: " << logfile << endl;  
  for(int i = 0 ; i < (int) crawlerportList.size() ; i ++ ) { 
    cout << "port of crawler: " << crawlerportList[i] << endl;
  }
}
*/

/*
  vector<string> str_list;
  char command[80];
  char parameter;
  cout<< "enter command: ";
  cin >> command;

  cout << "enter parameter: ";
  cin >> parameter;

  Parameter param("crawl_config");
  param.GetParameter(command, parameter, str_list);

  for(int i = 0 ; i < str_list.size() ; i++) {
    
    cout << str_list[i] << endl;
  }

}

*/














