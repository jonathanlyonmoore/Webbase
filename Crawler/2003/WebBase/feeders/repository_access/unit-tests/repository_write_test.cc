/** Unit-test for repository creation code
  *
  * Sriram Raghavan <rsram@cs.stanford.edu>
  */

#include "repository.h"
#include <iostream>
#include <vector>
#include <string>

int main(int argc, char *argv[])
{
  if (argc != 3) {
    cerr << "Usage: " << argv[0]  << " <root> <compr>" << endl;
    cerr << "      <root> = root of the repository " << endl;
    cerr << "      <compr> = 0 to create uncompressed rep, anything else to create a compressed rep" << endl;
    return -1;
  }
  
  const std::string rootDir = argv[1];
  const std::string compression = argv[2];
  bool compressedRepo = true;
  if (compression == "0") compressedRepo = false;
  
  
  vector<std::string> sites;
  sites.push_back("www.stanford.edu");
  sites.push_back("cs.stanford.edu");
  sites.push_back("db.stanford.edu");
  sites.push_back("graphics.stanford.edu");
  sites.push_back("networks.stanford.edu");

  char *suffixes[] = {"1", "2", "3", "4", "5", "6", "7", "8"};

  const char *buf = "Hello World";
  int bufLen = strlen(buf);

  repository *rep = new repository("repositoryName", compressedRepo, rootDir, 'w');
  
  for (int i = 0; i < sites.size(); i++) {
    for (int j = 0; j < 8; j++) {
      std::string url = "http://" + sites[i] + "/" + suffixes[j] + "test.html";
      cerr << "Trying to store " << url << endl;
      int rc = rep->store(sites[i], url, buf, bufLen);
      if (rc < 0) {
        cerr << "Error storing " << url << endl;
        cerr << "Repository return code = " << rc << endl;
      }
      cerr << "Successfully stored " << url << endl;
    }
    rep->end_site(sites[i], 8);
  }
  return 1;
}
