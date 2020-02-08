/*
   html_parser unit test
   Wang Lam <wlam@cs.stanford.edu> 4 Sep 2003
 */

#include "html_parser.h"

#include <iostream>

const char page1[] = "<HTML>"
"<HEAD></HEAD>"
"<BODY>"
"  <A href=\"http://shouldget01\">text</A>"
"  <A href=\"http://shouldget02\"></A>"
"  <!-- comment <A href=\"http://shouldnotget01\">text</A> -->"
"  <!-- comment -- -- <A href=\"http://shouldnotget02\">text</A> -->"
"  <!-- comment ---- <A href=\"http://shouldnotget03\">text</A> -->"
"  <!--comment----<A href=\"http://shouldnotget04\">text</A>-->"
"  <!--comment----<A href=\"http://shouldnotget05\">text</A>-->"
"<A href=\"http://shouldget03\"></A><a HREF=\"http://shouldget04\"></a>"
"<A HREF=\"http://shouldget05\"></A>"
"</BODY>"
"</HTML>";

int main()
{
   html_parser parser;
   parser.start_parse(page1, page1 + sizeof(page1));

   const char *urlbegin, *urlend;
   while (parser.get_link(urlbegin,urlend) >= 0) {
      std::string urlstring(urlbegin, urlend);
      std::cout << urlstring << std::endl;
   };

   parser.end_parse();
   return 0;
}

