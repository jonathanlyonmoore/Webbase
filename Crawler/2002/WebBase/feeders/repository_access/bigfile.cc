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
#include "bigfile.h"
#include "my_utility.h"
#include <zlib.h>

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <fstream.h>

// BEGIN SOLARIS PORT CHANGE
//#if defined(SUN)
//#  include <sys/statvfs.h>
//#else
//#  include <sys/vfs.h>
//#endif
#include <sys/statvfs.h>
// END SOLARIS PORT CHANGE

static inline const char* FILENAME_ONLY(const char* path)
{
  const char *s = strrchr(path, '/');
  return  ((s == NULL) ? path : s + 1);
}

static inline int CREATE_FILE(const char* name)
{
  int fd = creat(name, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd < 0) {
    return RC_BF_CANNOT_CREATE_FILE;
  }
  close(fd);

  return 0;
}

static inline int WRITE_BUF(FILE* file, const char* buf, int size) 
{
  int  rc;
  if (size == 0)  return 0;

  if ((rc = fwrite(buf, 1, size, file)) == 0) {
    return RC_BF_WRITE_ERROR;
  }

  return rc;
}

static inline int READ_BUF(FILE* file, char* buf, int size)
{
  int  rc;
  if (size == 0) return 0;

  if ((rc = fread(buf, 1, size, file)) == 0) {
    return RC_BF_READ_ERROR;
  }

  return rc;
}
		
bigfile::bigfile()
{
  bOpen = false;
  cur_file = NULL;
}

bigfile::~bigfile()
{
  if (bOpen) {
    mg->close(*this);
  }
}

int bigfile::add_chunk()
{
  int    rc;
  string chunkName;
  // find the directory to store the new chunk
  if ((rc = mg->allocate(chunkName)) < 0)  return rc;
  chunkName += name;
  chunkName += '.';
  chunkName += itoa(file.size());

  // create the new chunk file
  if ((rc = CREATE_FILE(chunkName.c_str())) < 0) return rc;

  // append the chunk
  file.push_back(chunkName);
  file_size.push_back(0);

  // write to the main file
  ofstream os(main_file.c_str());
  if (!os) return RC_BF_WRITE_ERROR;

  for (int i = 0; i < (int)file.size(); i++) {
    os << file[i] << endl;
  }
   
  return 0;
}

int bigfile::set_current(int file_num)
{
  assert(0 <= file_num && file_num < (int)file.size());
   
  FILE *tmp_file;
  char  tmp_mode[2];
   
  tmp_mode[0] = mode;
  tmp_mode[1] = 0;

  // open the specified file and close the current one
  tmp_file = fopen(file[file_num].c_str(), tmp_mode);
  if (tmp_file == NULL)  return RC_BF_CANNOT_OPEN_FILE;
  if (cur_file != NULL) fclose(cur_file);
  
  // set the file as current
  cur_file = tmp_file;
  this->file_num = file_num;
   
  return 0;
}


int bigfile::read(char* buf, int& size)
{
  int   rc, size1;
  long  pos;

  if (!bOpen) {
    return RC_BF_FILE_CLOSED;
  }
  if (mode != 'r') {
    return RC_BF_WRITE_ONLY;
  }
  if (size > max_file_size) {
    return RC_BF_BUFFER_TOO_LONG;
  }

  // get current positon
  pos = ftell(cur_file);
  if (pos < 0) return RC_BF_FILE_ACCESS_ERROR;

  size1 = file_size[file_num] - pos;
  if (size1 == 0 && file_num == (int)file.size() - 1) {
    return RC_BF_EOF;
  }

  // if we the read doesn't span on two files
  // read and exit
  if (size1 >= size || file_num == (int)file.size() - 1) {
    if ((rc = READ_BUF(cur_file, buf, size)) < 0)  return rc;
    size = rc;
    return 0;
  }

  // read the first half
  if ((rc = READ_BUF(cur_file, buf, size1)) < 0) return rc;
   
  // read the second half
  buf += size1;
  size -= size1;
  if ((rc = set_current(file_num + 1)) < 0)  return rc;
  if ((rc = READ_BUF(cur_file, buf, size)) < 0)   return rc;
  size = size1 + rc;

  return 0;
}


int bigfile::write(const char* buf, int size)
{
  int  rc;
  int  last_file = file.size() - 1;

  if (!bOpen) {
    return RC_BF_FILE_CLOSED;
  }
  if (mode == 'r') {
    return RC_BF_READ_ONLY;
  }
  if (size > max_file_size) {
    return RC_BF_BUFFER_TOO_LONG;
  }

  // In 'append' mode, open the last file
  if (mode == 'a' && file_num != last_file) {
    if ((rc = set_current(last_file)) < 0) {
      return rc;
    }
  }

  if (mode == 'a') {
    // if the current chunk is too big, append a new chunk
    // and move to it.
    if (file_size[file_num] + size > max_file_size) {
      // add a new chunk and set it as current
      if ((rc = add_chunk()) < 0)  return rc;
      if ((rc = set_current(file_num + 1)) < 0) return rc;
    }

    // append
    if ((rc = WRITE_BUF(cur_file, buf, size)) != size) {
      return rc;
    }
    file_size[file_num] += size;

    return 0;
  }

  if (mode == 'w') {
    int  size1;

    // get current position
    long pos = ftell(cur_file);
    if (pos < 0)  return RC_BF_FILE_ACCESS_ERROR;

    // if the buffer does not span on two files
    // write it and exit
    if (pos + size < file_size[file_num]) {
      if ((rc = WRITE_BUF(cur_file, buf, size)) != size)  return rc;
    }
    if (file_num == last_file && pos + size < max_file_size) {
      if ((rc = WRITE_BUF(cur_file, buf, size)) != size)  return rc;
      file_size[file_num] += size - (file_size[file_num] - pos);
      return 0;
    }

    // if current file is the last one, add a chunk at the end
    if (file_num == last_file) {
      if ((rc = add_chunk()) < 0) return rc;
    }
      
    // write the first half to the current file
    size1 = file_size[file_num] - pos;
    if ((rc = WRITE_BUF(cur_file, buf, size1)) != size1) {
      return rc;
    }
    buf += size1;
    size -= size1;

    // write the second half to the next file
    if ((rc = set_current(file_num + 1)) < 0)  return rc;
    if ((rc = WRITE_BUF(cur_file, buf, size)) != size) {
      return rc;
    }
    if (file_size[file_num] < size) {
      file_size[file_num] = size;
    }

    return 0;
  }

  return 0;
}

int bigfile::seek(int64 pos, int whence)
{
  int  rc, seek_from;
  long offset;

  if (!bOpen)  return RC_BF_FILE_CLOSED;

  switch (whence) {
  case SEEK_SET:
    if (pos < 0) {
	    return RC_BF_INVALID_PARAMETER;
    }
    seek_from = 0;
    goto seek_forward;
  case SEEK_END:
    if (pos > 0) {
	    return RC_BF_INVALID_PARAMETER;
    }
    seek_from = file.size() - 1;
    goto seek_backward;
  case SEEK_CUR:
    if (pos == 0) return 0;

    offset = ftell(cur_file);
    if (offset < 0) return RC_BF_FILE_ACCESS_ERROR;
	 
    if (pos > 0) {
	    // check whether the seek
	    // is within the current chunk
	    if (pos + offset < file_size[file_num]) {
        if (fseek(cur_file, pos, SEEK_CUR) < 0) {
          return RC_BF_FILE_ACCESS_ERROR;
        }
        return 0;
	    }

	    pos -= (file_size[file_num] - offset);
	    seek_from = file_num + 1;
	    goto seek_forward;
    } else {
	    if (pos + offset >= 0) {
        if (fseek(cur_file, pos, SEEK_CUR) < 0) {
          return RC_BF_FILE_ACCESS_ERROR;
        }
        return 0;
	    }
	    pos += offset;
	    seek_from = file_num - 1;
	    goto seek_backward;
    }
  default:
    return RC_BF_INVALID_PARAMETER;
  }

 seek_forward:
  for (int i = seek_from; i < (int)file_size.size(); i++) {
    if (pos < file_size[i]) {
      if ((rc = set_current(i)) < 0) return rc;
      if (fseek(cur_file, pos, SEEK_SET) < 0)  
        return RC_BF_FILE_ACCESS_ERROR;
	 
      return 0;
    }
      
    pos -= file_size[i];
  }

  return RC_BF_INVALID_PARAMETER;

 seek_backward:
  for (int i = seek_from; i >= 0; i--) {
    if (pos + file_size[i] >= 0) {
      if ((rc = set_current(i)) < 0) return rc;
      if (fseek(cur_file, pos, SEEK_END) < 0)  
        return RC_BF_FILE_ACCESS_ERROR;
	 
      return 0;
    }
      
    pos += file_size[i];
  }

  return RC_BF_INVALID_PARAMETER;
}

int  bigfile::tell(char* file, int& offset) const
{
  if (!bOpen)  return RC_BF_FILE_CLOSED;

  offset = ftell(cur_file);
  if (offset < 0) {
    return RC_BF_FILE_ACCESS_ERROR;
  }

  strcpy(file, this->file[file_num].c_str());

  return 0;
}

int64 bigfile::tell() const
{
  int64 pos;

  if (!bOpen)  return RC_BF_FILE_CLOSED;

  pos = ftell(cur_file);
  if (pos < 0) {
    return RC_BF_FILE_ACCESS_ERROR;
  }
     
  for (int i = 0; i < file_num; i++) {
    pos += file_size[i];
  }

  return pos;
}

bigfile_manager::bigfile_manager(const vector<string>* dirlist)
{
  struct stat buf;

  if (dirlist == NULL) return;

  for (int i = 0; i < (int)dirlist->size(); i++) {
    dir_list.push_back((*dirlist)[i]);
    if (*(dir_list[i].end()-1) != '/') {
      dir_list[i] += '/';
    }
    if (lstat(dir_list[i].c_str(), &buf) < 0) {
      LOG(("Error in reading disk list"));
      exit(1);
    }
    if (!S_ISDIR(buf.st_mode)) {
      LOG(("Invalid disk specification"));
      exit(1);
    }
  }
}

bigfile_manager::~bigfile_manager()
{
}

int bigfile_manager::create(const char* filename)
{
  int    rc;
  string path;

  // create main file
  ofstream os(filename);
  if (!os) {
    return RC_BF_CANNOT_CREATE_FILE;
  }

  // create the first file chunk
  if ((rc = allocate(path)) < 0) {
    return rc;
  }
  path += FILENAME_ONLY(filename);
  path += ".0";
   
  if ((rc = CREATE_FILE(path.c_str())) < 0) {
    return rc;
  }
 
  // write down the first chuck name
  os << path << endl;
   
  return 0;
}

int bigfile_manager::destroy(const char* filename)
{
  vector<string> filelist;

  // read all chunk names
  {
    ifstream is(filename);
    string   name;

    if (!is) {
      return RC_BF_CANNOT_OPEN_FILE;
    }
    is >> name;
    while (!is.eof()) {
      filelist.push_back(name);
      is >> name;
    }
  }

  // remove chunk files
  for (int i = 0; i < (int)filelist.size(); i++) {
    remove(filelist[i].c_str());
  }

  // remove main file
  remove(filename);
   
  return 0;
}

int bigfile_manager::open(bigfile& file, const char* filename, char mode)
{
  const char* s;
  int  rc, file_num = 0;
  FILE*  cur_file;
  char tmp_mode[2];

  ifstream is;
  string  chunkName;

  if (file.bOpen) {
    rc = RC_BF_FILE_OPEN;
    goto error_exit;
  }

  // initialize vector array
  file.file.erase(file.file.begin(), file.file.end());
  file.file_size.erase(file.file_size.begin(), 
                       file.file_size.end());
   
  // read the main file to find all associated file chunks
  is.open(filename);
  if (!is.is_open()) {
    rc = RC_BF_CANNOT_OPEN_FILE;
    goto error_exit;
  }
  is >> chunkName;
  while (!is.eof()) {
    file.file.push_back(chunkName);
    is >> chunkName;
  }
  if (file.file.size() <= 0) {
    rc = RC_BF_INVALID_FILE;
    goto error_exit;
  }

  // check the file sizes
  for (int i = 0; i < (int)file.file.size(); i++) {
    struct stat buf;
    if (stat(file.file[i].c_str(), &buf) < 0) {
      rc = RC_BF_CANNOT_OPEN_FILE;
      goto error_exit;
    }
    file.file_size.push_back(buf.st_size);
  }
   
  switch (mode) {
  case 'a':
    file_num = file.file.size() - 1;
    break;
  case 'w':
    file_num = 0;
    break;
  case 'r':
    file_num = 0;
    break;
  }

  s= file.file[file_num].c_str();
  tmp_mode[0] = mode, tmp_mode[1] = 0;
  cur_file = fopen(s, tmp_mode);
  if (cur_file == NULL) {
    rc = RC_BF_CANNOT_OPEN_FILE;
    goto error_exit;
  }

  file.main_file = filename;
  file.name      = FILENAME_ONLY(filename);

  file.bOpen     = true;
  file.mode      = mode;
  file.cur_file  = cur_file;
  file.file_num  = file_num;

  file.max_file_size = MAX_FILE_SIZE;
  file.mg = this;

  cerr << "Opened file number: " << file_num << endl;
  //  printf("Opened file number: %d\n", file_num );

  return 0;
   
 error_exit:
  return rc;
}

int bigfile_manager::close(bigfile& file)
{
  if (!file.bOpen) {
    return RC_BF_FILE_CLOSED;
  }
      
  // close open files
  if (file.cur_file != NULL) {
    fclose(file.cur_file);
  }
   
  // erase file list vector
  file.file.erase(file.file.begin(), file.file.end());
  file.file_size.erase(file.file_size.begin(),
                       file.file_size.end());

  file.bOpen = false;
   
  return 0;
}


// BEGIN SOLARIS PORT CHANGE
// #ifdef SUN
// #  define STATFS statvfs
// #else
// #  define STATFS statfs
// #endif
#define STATFS statvfs
// END SOLARIS PORT CHANGE

int bigfile_manager::allocate(string& dir)
{
  struct STATFS statbuf;
  vector<long>   disksize(dir_list.size());
  double         ratio;
  int            rc;

  //
  // check the free space of each directory
  //
  for (int i = 0; i < (int)dir_list.size(); i++) {
    if ((rc = STATFS(dir_list[i].c_str(), &statbuf)) < 0) {
      return RC_BF_FILE_ACCESS_ERROR;
    }
    ratio = (double)1024 / (double)statbuf.f_bsize;
    disksize[i] = (long)(statbuf.f_bavail / ratio);
  }
      
  for (int i = 0; i < (int)dir_list.size(); i++) {
    if (disksize[i] > (MAX_FILE_SIZE + 1024 - 1) / 1024) {
      dir = dir_list[i];
      return 0;
    }
  }
   
  return RC_BF_DISK_FULL;
}


repository::repository(const char* name, bool compressed, const vector<string>* dirs, char mode)
{
  this->compressed = compressed;        // the repository is to be stored compressed
  int  rc;
  mg = new bigfile_manager(dirs);
  file = new bigfile;

  if (mode != 'r' && dirs == NULL) {
    LOG(("Invalid parameter for repository constructor"));
    exit(1);
  }

  // open repository file
  if ((rc = mg->open(*file, name, mode)) < 0) {
    if (mode == 'r' || rc != RC_BF_CANNOT_OPEN_FILE) {
      LOG(("Cannot open repository %d", rc));
      exit(1);
    }
    if ((rc = mg->create(name)) < 0) {
      LOG(("Cannot create repository"));
      exit(1);
    }
    if ((rc = mg->open(*file, name, mode)) < 0) {
      LOG(("Cannot open repository"));
      exit(1);
    }
  }
   
  // prepare compression buffer
  comp_buf = new char[COMP_BUFFER_SIZE];
}

repository::~repository()
{
  delete mg;
  delete file;
  delete [] comp_buf;
}

int repository::seek(int64 offset, int whence)
{
  return file->seek(offset, whence);
}

int64 repository::tell() const
{
  return file->tell();
}

int repository::store(const char* name, const char* buf, int size)
{
  int namelen = strlen(name);
  int headerlen = namelen + 20;
  int complen, rc;
  unsigned tmpNum;


  // compress the data
  if (compressed) { 
    complen = COMP_BUFFER_SIZE - headerlen;
    rc = compress((unsigned char*)comp_buf + headerlen, (unsigned long*)&complen,
                  (unsigned char*)buf,      (unsigned long)size);
    if (rc != Z_OK) {
      return RC_BF_COMPRESS_ERROR;
    }
  }     // do not compress the data. Set compressed length = original page length
  else {
    complen = size;     // compressed size = page size
    memcpy(comp_buf + headerlen, buf, size);
  }

  // attach header
  tmpNum = htonl(FILE_MAGIC_NUMBER);
  memcpy(comp_buf +  0, &tmpNum, 4);
  tmpNum = htonl(time(NULL));
  memcpy(comp_buf +  4, &tmpNum, 4);
  tmpNum = htonl(size);
  memcpy(comp_buf +  8, &tmpNum, 4);
  tmpNum = htonl(complen);
  memcpy(comp_buf + 12, &tmpNum, 4);
  tmpNum = htonl(namelen);
  memcpy(comp_buf + 16, &tmpNum, 4);
  memcpy(comp_buf + 20, name, namelen);
   
  // write to disk
  if ((rc = file->write(comp_buf, headerlen + complen)) < 0) {
    return rc;
  }
   
  return 0;
}


int repository::read(char* name, time_t& time, char* buf, int& size, bool uncompr = true)
{
  unsigned header[5];
  int      rc, tmpSize;
  int64    pos; 

  // record current position
  if ((pos = file->tell()) < 0)  return pos;

  // read header
  tmpSize = 20;
  if ((rc = file->read((char*)header, tmpSize)) < 0)  return rc;
  if (tmpSize < 20)  return RC_BF_PREMATURE_EOF;

  // change byte orders
  header[0] = ntohl(header[0]); // magic code
  time      = ntohl(header[1]); // time
  header[2] = ntohl(header[2]); // file size
  header[3] = ntohl(header[3]); // compressed size
  header[4] = ntohl(header[4]); // url length

  // check file integrity
  if (header[0] != (unsigned)FILE_MAGIC_NUMBER) {
    cerr << "Header error : Mismatch in magic number" << endl;
    goto error_exit;
  }
  if (header[2] > (unsigned)MAX_FILE_SIZE*2 || header[3] > (unsigned)MAX_FILE_SIZE) {
    cout << "Header[2] = " << header[2] << endl;
    cout << "Header[3] = " << header[3] << endl;
    cout << "Header error : Compressed or uncompressed file sizes not valid" << endl;
    goto error_exit;
  }
  if (header[4] > 10*1024) {
    cout << "Header error : Url length invalid" << endl;
    goto error_exit;
  }
      
  // read url
  tmpSize = header[4];
  if ((rc = file->read(name, tmpSize)) < 0) return rc;
  if ((unsigned)tmpSize < header[4]) return RC_BF_PREMATURE_EOF;
  name[tmpSize] = 0;

  // read page
  tmpSize = header[3];

  if ( !compressed && !uncompr) {
    LOG(("Compressed pages requested when pages not stored compressed"));
    return 0;
  }

  if (compressed && uncompr) {
    if ((rc = file->read(comp_buf, tmpSize)) < 0) return rc;
    if ((unsigned)tmpSize < header[3]) return RC_BF_PREMATURE_EOF;
     
    rc = uncompress((unsigned char*)buf, (unsigned long*)&size, 
                    (unsigned char*)comp_buf, tmpSize);
    if (rc != Z_OK || header[2] != (unsigned)size) {
      cout << "Size after uncompression does not match size mentioned in header" << endl;
      goto error_exit;
    }
  } else { // either repository is compressed and pages are requested compressed or
    // repository is uncompressed and uncompressed pages are requested. In either case
    // read directly into buf
    if ((rc = file->read(buf, tmpSize)) < 0) return rc;
    if ((unsigned)tmpSize < header[3]) return RC_BF_PREMATURE_EOF;
    size = tmpSize;
  }

  return 0;

 error_exit:
  LOG(("File corrupted at %lld", pos));
  if ((rc = file->seek(pos + 1, SEEK_SET)) < 0) return rc;
  return resync(name, time, buf, size);
}


int repository::resync(char* name, time_t& time, 
                       char* buf, int& size)
{
  const int tmpBufSize = 10*1024;
  int64 pos;
  int   rc, tmpSize;
  char  magic[4];
  char  tmpBuf[tmpBufSize];

  // copy magic number in network order
  {
    unsigned tmp = htonl(FILE_MAGIC_NUMBER);
    memcpy(magic, &tmp, 4);
  }

 read_loop:
  // get current file position
  pos = file->tell();
  if (pos < 0) return pos;
     
  tmpSize = tmpBufSize;
  if ((rc = file->read(tmpBuf, tmpSize)) < 0)  return rc;
  if (tmpSize < tmpBufSize)   return RC_BF_PREMATURE_EOF;

  for (int i = 0; i <= tmpSize - 4; i++) {
    if (memcmp(tmpBuf+i, magic, 4) == 0) {
      if ((rc = file->seek(pos+i, SEEK_SET)) < 0)  return rc;
      return read(name, time, buf, size);
    }
  }
  goto read_loop;
}


