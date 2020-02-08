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
// Convert unsigned to signed docid
// maybe make sure second field is positive
// flags:
// -f check first field (DocID)
// -s check second field for < 0
//    Gary Wesley <gary@db.stanford.edu> 9/1
#include <iostream.h>
#include <iomanip>
#include <stdio.h>
#include <unistd.h>

#define MAXSECONDFIELD  1000000000000  // for pagelength (unused)

int main(int argc, char *argv[])
{ 
  unsigned long long urlhash;
  signed   long long urlhashsigned;

  unsigned long  long  secondField;
  signed   long  long  secondFieldsigned; 
  //signed   long      secondFieldsignedl;

  bool  checkFirstField  = false;
  bool  checkSecondField = false;

  int numbadURLtossed    = 0;
  int numbad2ndtossed    = 0;
  int numinspected       = 0;
  int c, errflg = 0;
  
  while ((c = getopt(argc, argv, "fs")) != EOF) {
    switch (c) {
    case 'f':
      checkFirstField = true;
      cerr << " checkFirstField ";
      break;
    case 's':
      checkSecondField = true;
      cerr << " checkSecondField ";
      break;
    case '?':
      errflg++;
    }
  }
 
  while( cin >> urlhash, cin >> secondField ){
    urlhashsigned = (signed long long ) urlhash;
    numinspected++;
    if( ! checkFirstField ||
	//urlhashsigned != 0  && 
	( urlhashsigned > 10000 | urlhashsigned < 0 )){  
      
      
      cout.width( 20 );cout.fill(' ');
      
      secondFieldsigned =  (signed long  long)  secondField;
      //secondFieldsignedl  =  (signed      long)  secondField;
      
      if(! checkSecondField ||
	 ( secondFieldsigned >= 0 
	   //	  &&  secondFieldsigned < MAXSECONDFIELD // size check
	   )){
	
	cout << urlhashsigned << "\t"; 
	cout.width( 15 );cout.fill('0');
	cout << secondFieldsigned << endl;
      }
      else{  // yes we check it
	if( secondFieldsigned < 0 ){
	  numbad2ndtossed++;
	  //  cerr << "toss>" << secondFieldsigned;
	}
        else{
	  cout <<  urlhashsigned << "\t0\n";
	}
      }	
    }
    else 
      if( checkFirstField && 
	  (( urlhashsigned == 0 ) |
	   ( urlhashsigned < 10000 && urlhashsigned > 0 ))) 
	numbadURLtossed++;    
  }
  if( checkFirstField ){ 
    cerr << "rejected: " << numbadURLtossed << " bad/0 urlhashs out of " ;
  }
  
  cerr << numinspected << " items ";
  
  if( checkFirstField ){
    cerr << "... which is ";
    cerr << setprecision(5) <<  ( (float)numbadURLtossed / (float) numinspected) * 100.0 << "%";
  }
  
  if( checkSecondField ){
    cerr << setprecision(3) <<  " rejected 2nd field: " << numbad2ndtossed;
    cerr << endl;
    return numbadURLtossed;
  }
}
