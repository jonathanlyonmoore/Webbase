/*
 * TypeDetector unit test
 */

#include <iostream>
#include <string>
#include <vector>

#include "TypeDetector.h"

int main() {
  TypeDetector t;

  cout << TypeDetector::makeLower("Hello World") << endl;

  TypeDetector::init();

  int i;
  for(i = TypeDetector::ANY; i < TypeDetector::_TYPE_LEN; i++) {
    cout << TypeDetector::TypeNames[i] << endl;
    cout << TypeDetector::NameTypes[TypeDetector::TypeNames[i]] << endl;
  }

  string urls[] = {
    "http://www.mp3.com",
    "http://www.mp3.com/",
    "http://www.mp3.com/index.html",
    "http://www.mp3.com/foo.mp3",
    "http://www.mp3.com/mp3",
    "http://www.mp3.com/foo.mp3/",
    "http://www.mp3.com/a.gif",
    "http://www.mp3.com/mp3",
    "http://www.mp3.com/index.htm",
    "http://www.mp3.com/index.HTM"
    "http://www.mp3.com/robots.txt",
    "http://www.mp3.com/robots.TXT"
  };


  int len = sizeof(urls)/sizeof(urls[0]);
  for(i = 0; i < len; i++) {
    cout << urls[i] << '\t' 
	 << TypeDetector::extractExtension(urls[i]) << '\t'
	 << TypeDetector::TypeNames[t.getTypeOf(urls[i])] << endl;
  }	   

}
