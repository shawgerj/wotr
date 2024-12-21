#include <iostream>
#include <vector>
#include <system_error>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "wotr.h"

Wotr::Wotr(const char* logname) {
  _logname = std::string(logname);
  _offset = 0;

  // open wotr valuelog
  if ((_log = open(logname, O_RDWR | O_CREAT | O_APPEND, S_IRWXU)) < 0) {
    std::cout << "Error opening wotr" << std::endl;
    throw std::system_error(errno, std::generic_category(), logname);
  }
  lseek(_log, 0, SEEK_END);

  struct stat stbuf;
  if (fstat(_log, &stbuf) < 0) {
    throw std::system_error(errno, std::generic_category(), logname);
  }
  _offset = stbuf.st_size;

  std::string stats_fname = _logname + ".stats";
  std::cout << stats_fname << std::endl;
  // open stats logging file
  if ((_statslog = open(stats_fname.c_str(), O_RDWR | O_CREAT | O_APPEND, S_IRWXU)) < 0) {
    std::cout << "Error opening stats file" << std::endl;
    throw std::system_error(errno, std::generic_category(), logname);
  }

  _statsstart = (char*)malloc(STAT_BUF_SIZE * sizeof(char));
  _statsptr = _statsstart;

  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  _inception = static_cast<size_t>(ts.tv_sec) * 1000000000 + ts.tv_nsec;
}


// use check_bounds() before calling this function to ensure offset is in range
int Wotr::get_entry(size_t offset, Entry* entry) {
  // read the header
  // use pread for thread safety
  item_header header;
  if (pread(_log, (char*)&header, sizeof(item_header), (ssize_t)offset) < 0) {
    std::cout << "get_recovery_entry: bad read header: "
	      << strerror(errno) << std::endl;
    return -1;
  }

  entry->cfid = header.cfid;
  entry->ksize = header.ksize;
  entry->vsize = header.vsize;
  entry->key_offset = offset + sizeof(item_header);
  entry->value_offset = entry->key_offset + header.ksize;
  entry->size = sizeof(item_header) + header.ksize + header.vsize;
  entry->offset = offset;
  
  return 0;
}

int Wotr::safe_write(int fd, const char* data, size_t size) {
  const char* src = data;
  size_t ret, remaining;
  remaining = size;

  while (remaining != 0) {
    ret = write(fd, src, remaining);
    if (ret < 0) {
      if (errno == EINTR) { continue; } // rocksdb checks this, so I will do
      return -1;
    }
    remaining -= ret;
    src += ret;
  }
  return 0;
}

// append to log
ssize_t Wotr::WotrWrite(std::string& logdata) {
  std::lock_guard<std::mutex> lock(_lock);
  
  if (lseek(_log, _offset, SEEK_SET) < 0) {
    std::cout << "wotrwrite: Error seeking log" << std::endl;
    return -1;
  }
  size_t bytes_to_write = logdata.size();

  if (safe_write(_log, logdata.data(), bytes_to_write) < 0) {
    std::cout << "wotrwrite write data: " << strerror(errno) << std::endl;
    return -1;
  }
  
  ssize_t ret = _offset;
  _offset += logdata.size();
  return ret;
}

int Wotr::WotrGet(size_t offset, char** data, size_t* len) {
  if (check_bounds(offset) < 0) {
    return -1;
  }
  
  Entry entry;
  if (get_entry(offset, &entry) != 0) {
    std::cout << "wotrget: problem getting entry" << std::endl;
    return -1;
  }

  char *vbuf = (char*)malloc(entry.vsize * sizeof(char));
  
  if (pread(_log, vbuf, entry.vsize, entry.value_offset) < 0) {
    std::cout << "wotrget: read value: " << strerror(errno) << std::endl;
    std::cout << "attempted offset: " << entry.value_offset << "size: " << entry.vsize << std::endl;
    return -1;
  }
  
  *data = vbuf;
  *len = entry.vsize;

  return 0;
}

int Wotr::WotrPGet(size_t offset, char** data, size_t len) {
  char *vbuf = (char*)malloc(len * sizeof(char));
  if (pread(_log, vbuf, len, (ssize_t)offset) < 0) {
    std::cout << "wotrpget read value: " << strerror(errno) << std::endl;
    return -1;
  }
  *data = vbuf;
  return 0;
}

int Wotr::Sync() {
  return fsync(_log);
}

ssize_t Wotr::Head() {
  std::lock_guard<std::mutex> lock(_lock);
  return _offset;
}

int Wotr::CloseAndDestroy() {
  fsync(_log);
  close(_log);
  return unlink(_logname.c_str());
}

int Wotr::check_bounds(size_t offset) {
  struct stat stbuf;
  if (fstat(_log, &stbuf) < 0 || offset >= stbuf.st_size) {
    std::cout << "wotr check bounds fail" << strerror(errno) << std::endl;
    std::cout << "attempted offset: " << offset << std::endl;
    std::cout << "bounds: " << stbuf.st_size << std::endl;
    return -1;
  }

  return 0;
}

WotrIter::WotrIter(Wotr& wotr)
  : w(wotr), _offset(0) {}

int WotrIter::read(Entry* entry) {
  if (w.check_bounds(_offset) < 0) {
    std::cout << "wotr iter read out of bounds" << std::endl;
    return -1;
  }
  
  w.get_entry(_offset, &current);
  memcpy(entry, &current, sizeof(Entry));
  return 0;
}

void WotrIter::set_offset(size_t offset) {
  _offset = offset;
}

// n.b. always read() between calls to next() to load the entry at _offset
//      into "current", or this will break. Design could be improved later...
void WotrIter::next() {
  _offset += current.size;
}

char* WotrIter::read_key() {
  char* key = (char*)malloc(sizeof(char) * current.ksize);
  if (pread(w._log, key, current.ksize, current.key_offset) < 0) {
    std::cout << "read_key: pread error" << strerror(errno) << std::endl;
    return nullptr;
  }
  return key;
}

char* WotrIter::read_value() {
  char* value = (char*)malloc(sizeof(char) * current.vsize);
  if (pread(w._log, value, current.vsize, current.value_offset) < 0) {
    std::cout << "read_value: pread error" << strerror(errno) << std::endl;
    return nullptr;
  }
  return value;

}

uint32_t WotrIter::GetCfID() {
  return current.cfid;
}
