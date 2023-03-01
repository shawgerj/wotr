#include <iostream>
#include <vector>
#include <system_error>
#include <mutex>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include "wotr.h"

Wotr::Wotr(const char* logname) {
  _logname = std::string(logname);
  _offset = 0;
  _db_counter = 0;
  _seq = 0;
  if ((_log = open(logname, O_RDWR | O_CREAT | O_APPEND, S_IRWXU)) < 0) {
    std::cout << "Error opening wotr" << std::endl;
    throw std::system_error(errno, std::generic_category(), logname);
  }
}

int Wotr::Register(std::string path) {
  std::scoped_lock lock(_m);
  int ret = _db_counter;
  _dbs.insert(std::make_pair(_db_counter, path));
  _db_counter++;

  return ret;
}

void Wotr::UnRegister(int ident) {
  _dbs.erase(ident);
}

int Wotr::NumRegistered() {
  return _dbs.size();
}

int safe_write(int fd, const char* data, size_t size) {
  const char* src = data;
  size_t left = size;

  while (left != 0) {
    size_t bytes_to_write = left;

    ssize_t done = write(fd, src, bytes_to_write);
    if (done < 0) {
      if (errno == EINTR) { // rocksdb does this check, so I will do
        continue;
      }
      return -1;
    }
    left -= done;
    src += done;
  }
  return 0;
}

// append to log
logoffset_t* Wotr::WotrWrite(std::string& logdata, int flush) {
  std::scoped_lock lock(_m);
  if (lseek(_log, _offset, SEEK_SET) < 0) {
    std::cout << "wotrwrite: Error seeking log" << std::endl;
    return nullptr;
  }

  if (safe_write(_log, logdata.data(), logdata.size()) < 0) {
    std::cout << "wotrwrite write data: " << strerror(errno) << std::endl;
    return nullptr;
  }

  if (flush && fsync(_log) < 0) {
    return nullptr;
  }

  logoffset_t* ret = new logoffset_t;
  ret->offset = (size_t)_offset;
  ret->seq = _seq;
  
  _offset += logdata.size();
  _versions.insert(std::make_pair(_seq, _offset));
  _seq++;
  return ret;
}

int Wotr::WotrGet(size_t offset, char** data, size_t* len, size_t version) {
  // if version = 0, any offset is accepted
  if (version != 0 && offset >= (size_t)(_versions[version])) {
    std::cout << "bad version number" << std::endl;
    return -1;
  }
  
  item_header *header = (item_header*)malloc(sizeof(item_header));

  // read the header
  // use pread for thread safety
  if (pread(_log, (char*)header, sizeof(item_header), (off_t)offset) < 0) {
    std::cout << "bad read header: " << strerror(errno) << std::endl;
    return -1;
  }

  // don't need the CF id right now, but it is in the header
  char *kbuf = (char*)malloc(header->ksize * sizeof(char));
  char *vbuf = (char*)malloc(header->vsize * sizeof(char));

  off_t key_offset = (off_t)offset + sizeof(item_header);
  off_t value_offset = key_offset + header->ksize;

  if (pread(_log, kbuf, header->ksize, key_offset) < 0) {
    std::cout << "wotrget read key: " << strerror(errno) << std::endl;
    return -1;
  }

  if (pread(_log, vbuf, header->vsize, value_offset) < 0) {
    std::cout << "wotrget read value: " << strerror(errno) << std::endl;
    return -1;
  }

  *data = vbuf;
  *len = header->vsize;
  free(kbuf);
  free(header);

  return 0;
}

int Wotr::Flush() {
  return fsync(_log);
}

int Wotr::CloseAndDestroy() {
  close(_log);
  return unlink(_logname.c_str());
}
