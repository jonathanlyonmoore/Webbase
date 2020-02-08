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
/*********************************************************************************
 *                          Utils
 * 
 * Bunch of utility functions defined in the Utils class
 *
 * Author : Sriram Raghavan (rsram@cs.stanford.edu)
 *********************************************************************************/

#include "Utils.h"

// This is required to get things to build using older version of 
// the libstdc++-libc6 library (specifically, on the WB20X machines)
void __rethrow() { }

static bool entityMapInit = false;
static const int MAX_SERVER_PIECES = 100;
static char *serverPieces[MAX_SERVER_PIECES];
static hash_map<const char *, unsigned char, hash<const char *>, str_equal> entity_map;
static entityPair entities[] =
  {
    { "lt",	      '<' } ,
    { "gt",	      '>' } ,
    { "amp",	      '&' } ,
    { "quot",	      '"' } ,
    { "trade",	      153 } , /* trade mark */
    { "nbsp",         160 } , /* non breaking space */
    { "iexcl",        161 } , /* inverted exclamation mark */
    { "cent",         162 } , /* cent sign */
    { "pound",        163 } , /* pound sign */
    { "curren",       164 } , /* currency sign */
    { "yen",          165 } , /* yen sign */
    { "brvbar",       166 } , /* broken vertical bar, (brkbar) */
    { "sect",         167 } , /* section sign */
    { "uml",          168 } , /* spacing diaresis */
    { "copy",         169 } , /* copyright sign */
    { "ordf",         170 } , /* feminine ordinal indicator */
    { "laquo",        171 } , /* angle quotation mark, left */
    { "not",          172 } , /* negation sign */
    { "shy",          173 } , /* soft hyphen */
    { "reg",          174 } , /* circled R registered sign */
    { "hibar",        175 } , /* spacing macron */
    { "deg",          176 } , /* degree sign */
    { "plusmn",       177 } , /* plus-or-minus sign */
    { "sup2",         178 } , /* superscript 2 */
    { "sup3",         179 } , /* superscript 3 */
    { "acute",        180 } , /* spacing acute (96) */
    { "micro",        181 } , /* micro sign */
    { "para",         182 } , /* paragraph sign */
    { "middot",       183 } , /* middle dot */
    { "cedil",        184 } , /* spacing cedilla */
    { "sup1",         185 } , /* superscript 1 */
    { "ordm",         186 } , /* masculine ordinal indicator */
    { "raquo",        187 } , /* angle quotation mark, right */
    { "frac14",       188 } , /* fraction 1/4 */
    { "frac12",       189 } , /* fraction 1/2 */
    { "frac34",       190 } , /* fraction 3/4 */
    { "iquest",       191 } , /* inverted question mark */
    { "Agrave",       192 } , /* capital A, grave accent */ 
    { "Aacute",       193 } , /* capital A, acute accent */ 
    { "Acirc",        194 } , /* capital A, circumflex accent */ 
    { "Atilde",       195 } , /* capital A, tilde */ 
    { "Auml",         196 } , /* capital A, dieresis or umlaut mark */ 
    { "Aring",        197 } , /* capital A, ring */ 
    { "AElig",        198 } , /* capital AE diphthong (ligature) */ 
    { "Ccedil",       199 } , /* capital C, cedilla */ 
    { "Egrave",       200 } , /* capital E, grave accent */ 
    { "Eacute",       201 } , /* capital E, acute accent */ 
    { "Ecirc",        202 } , /* capital E, circumflex accent */ 
    { "Euml",         203 } , /* capital E, dieresis or umlaut mark */ 
    { "Igrave",       205 } , /* capital I, grave accent */ 
    { "Iacute",       204 } , /* capital I, acute accent */ 
    { "Icirc",        206 } , /* capital I, circumflex accent */ 
    { "Iuml",         207 } , /* capital I, dieresis or umlaut mark */ 
    { "ETH",          208 } , /* capital Eth, Icelandic (Dstrok) */ 
    { "Ntilde",       209 } , /* capital N, tilde */ 
    { "Ograve",       210 } , /* capital O, grave accent */ 
    { "Oacute",       211 } , /* capital O, acute accent */ 
    { "Ocirc",        212 } , /* capital O, circumflex accent */ 
    { "Otilde",       213 } , /* capital O, tilde */ 
    { "Ouml",         214 } , /* capital O, dieresis or umlaut mark */ 
    { "times",        215 } , /* multiplication sign */ 
    { "Oslash",       216 } , /* capital O, slash */ 
    { "Ugrave",       217 } , /* capital U, grave accent */ 
    { "Uacute",       218 } , /* capital U, acute accent */ 
    { "Ucirc",        219 } , /* capital U, circumflex accent */ 
    { "Uuml",         220 } , /* capital U, dieresis or umlaut mark */ 
    { "Yacute",       221 } , /* capital Y, acute accent */ 
    { "THORN",        222 } , /* capital THORN, Icelandic */ 
    { "szlig",        223 } , /* small sharp s, German (sz ligature) */ 
    { "agrave",       224 } , /* small a, grave accent */ 
    { "aacute",       225 } , /* small a, acute accent */ 
    { "acirc",        226 } , /* small a, circumflex accent */ 
    { "atilde",       227 } , /* small a, tilde */
    { "auml",         228 } , /* small a, dieresis or umlaut mark */ 
    { "aring",        229 } , /* small a, ring */
    { "aelig",        230 } , /* small ae diphthong (ligature) */ 
    { "ccedil",       231 } , /* small c, cedilla */ 
    { "egrave",       232 } , /* small e, grave accent */ 
    { "eacute",       233 } , /* small e, acute accent */ 
    { "ecirc",        234 } , /* small e, circumflex accent */ 
    { "euml",         235 } , /* small e, dieresis or umlaut mark */ 
    { "igrave",       236 } , /* small i, grave accent */ 
    { "iacute",       237 } , /* small i, acute accent */ 
    { "icirc",        238 } , /* small i, circumflex accent */ 
    { "iuml",         239 } , /* small i, dieresis or umlaut mark */ 
    { "eth",          240 } , /* small eth, Icelandic */ 
    { "ntilde",       241 } , /* small n, tilde */ 
    { "ograve",       242 } , /* small o, grave accent */ 
    { "oacute",       243 } , /* small o, acute accent */ 
    { "ocirc",        244 } , /* small o, circumflex accent */ 
    { "otilde",       245 } , /* small o, tilde */ 
    { "ouml",         246 } , /* small o, dieresis or umlaut mark */ 
    { "divide",       247 } , /* division sign */
    { "oslash",       248 } , /* small o, slash */ 
    { "ugrave",       249 } , /* small u, grave accent */ 
    { "uacute",       250 } , /* small u, acute accent */ 
    { "ucirc",        251 } , /* small u, circumflex accent */ 
    { "uuml",         252 } , /* small u, dieresis or umlaut mark */ 
    { "yacute",       253 } , /* small y, acute accent */ 
    { "thorn",        254 } , /* small thorn, Icelandic */ 
    { "yuml",         255 } , /* small y, dieresis or umlaut mark */
    { 0, 0 }
  };

void Utils::threadSleep(long long millis) {

  if(millis <= 0)
    return;

  long long wakeup = currentTimeMillis() + millis; // absolute time to wake up
  
  struct timespec t;
  t.tv_sec = wakeup / 1000;
  t.tv_nsec = (wakeup % 1000) * 1000000; // from millis to nanos

  pthread_cond_t cvp;
  pthread_mutex_t mutex;
  pthread_cond_init(&cvp, NULL);
  pthread_mutex_init(&mutex, NULL);

  //  cout << "GOING to sleep" << endl;
  pthread_cond_timedwait(&cvp, &mutex, &t);
  //  cout << "AWAKEN!" << endl;
}


bool Utils::readn(int sockfd, char *buf, int nbytes)
{
  int nread, nleft = nbytes;

  while (nleft > 0) {
    nread = read(sockfd, buf, nleft);
    if (nread <= 0) return(false);
    nleft -= nread;
    buf += nread;
  }
  return(true);
}


bool Utils::writen(int sockfd, char *buf, int nbytes)
{
  int nwritten, nleft = nbytes;

  while (nleft > 0) {
    nwritten = write(sockfd, buf, nleft);
    if (nwritten <= 0) return(false);
    nleft -= nwritten;
    buf += nwritten;
  }
  return(true);
}


long long Utils::currentTimeMillis()
{
  struct timeval tv;

  gettimeofday(&tv, NULL);
  //  cout << "Sec is: " << tv.tv_sec << ", usec is " << tv.tv_usec << endl;
  return ((long long)tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}


const char * Utils::skipHeader(const char *p) 
{
  while(*p != '\0') {
    if(*p == '\n' && *(p+1) == '\n')
      return p + 2;
    if(*p == '\r' && *(p+1) == '\n' && *(p+2) == '\r' && *(p+3) == '\n')
      return p + 4;
    p++;
  }
  return p;
}


unsigned char Utils::translate(char *entity)
{
  if (! entityMapInit) {
      for (int i = 0; entities[i].entity; i++)
        entity_map[entities[i].entity] = entities[i].equiv;
      entityMapInit = true;
  }
  hash_map<const char *, unsigned char, hash<const char *>, str_equal>::iterator iter;
  if (!entity || !*entity) return(' ');  
  if ( (iter = entity_map.find(entity)) != entity_map.end()) 
    return((*iter).second);
  if (*entity == '#' && isdigit(entity[1])) 
    return (atoi(entity + 1));
  return(' ');
}


// WARNING: Assumes lots of things about incoming url strings
// WARNING: Not thread safe, as it uses a static "serverPieces" variable.
bool Utils::reverseServer(char *url)
{
  char *ptr = url;
  char *serverEnd = strstr((const char *)ptr, "/");
  if (serverEnd == NULL) serverEnd = url + strlen(url);
  char endChar = *serverEnd;
  int serverLen = serverEnd - ptr;
  char *temp = new char[serverLen+1];
  temp[serverLen] = '\0';
  memcpy(temp, ptr, serverLen);

  // tokenize and keep list of pointers to tokens
  int tokCount = 0;
  char *token = strtok(temp, ".");
  while (token) {
    if (tokCount >= MAX_SERVER_PIECES) return(false);
    serverPieces[tokCount++] = token;
    token = strtok(NULL, ".");
  }
  while (tokCount--) {
    int toklen = strlen(serverPieces[tokCount]);
    memcpy(ptr, serverPieces[tokCount], toklen);
    ptr[toklen] = '.';
    ptr += toklen+1;
  }
  *serverEnd = endChar;
  delete(temp);
  return(true);
}
