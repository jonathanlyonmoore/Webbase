/*
 * PathGenerator unit test
 */

#include <iostream>
#include <string>

#include "PathGenerator.h"

int main() {
  PathGenerator pg("/dfs/webbase/");

  string urls[] = {
    "abc.mp3.com",
    ".abc.mp3.com", 
    "abc.mp3.com.",
    ".abc.mp3.com.",
    "abc.cs.berkeley.edu",
    " ",
    ""
};

  int i;
  int len = sizeof(urls)/sizeof(urls[0]);
  for(i = 0; i < len; i++) {
    cout << urls[i] << '\t' 
	 << pg.getPathFor(urls[i], TypeDetector::TEXT) << '\t'
	 << pg.getPathFor(urls[i], TypeDetector::AUDIO) << endl;
  }	   

}
