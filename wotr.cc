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

std::mutex write_mutex;

Wotr::Wotr(const char* logname) {
  _logname = std::string(logname);
  if ((_log = open(logname, O_RDWR | O_CREAT | O_APPEND, S_IRWXU)) < 0) {
    std::cout << "Error opening wotr" << std::endl;
    throw std::system_error(errno, std::generic_category(), logname);
  }
}

int safe_write(int fd, const char* data, size_t size) {
  ssize_t written = 0;

  for (ssize_t total = 0; total < (ssize_t)size; total += written) {
    written = write(fd, data + written, size - written);
    if (written < 0)
      return -1;
  }
  return 0;
}

int safe_read(int fd, char* buf, size_t size) {
  ssize_t haveread, total;
  haveread = total = 0;

  for (total = 0; total < (ssize_t)size; total += haveread) {
    haveread = read(fd, buf + haveread, size - haveread);
    if (haveread < 0)
      return -1;
  }

  return 0;
}

size_t Wotr::CurrentOffset() {
  off_t off;
  if ((off = lseek(_log, 0, SEEK_END)) < 0) {
    std::cout << "wotroffset: Error seeking log" << std::endl;
    return -1;
  }
  return (size_t)off;
}

// append to log
int Wotr::WotrWrite(std::string& logdata, int flush) {
  std::lock_guard<std::mutex> guard(write_mutex);
  if (lseek(_log, 0, SEEK_END) < 0) {
    std::cout << "wotrwrite: Error seeking log" << std::endl;
    return -1;
  }

  if (safe_write(_log, logdata.data(), logdata.size()) < 0) {
    std::cout << "wotrwrite write data: " << strerror(errno) << std::endl;
    return -1;
  }

  if (flush) {
    return fsync(_log);
  }

  return 0;
}

int Wotr::WotrGet(size_t offset, char** data, size_t* len) {
  item_header *header = (item_header*)malloc(sizeof(item_header));
    
  if (lseek(_log, offset, SEEK_SET) < 0) {
    std::cout << "wotrget read seek: " << strerror(errno) << std::endl;
    return -1;
  }

  if (safe_read(_log, (char*)header, sizeof(item_header)) < 0) {
    std::cout << "wotrget read header: " << strerror(errno) << std::endl;
    return -1;
  }

  char *kbuf = (char*)malloc(header->ksize * sizeof(char));
  char *vbuf = (char*)malloc(header->vsize * sizeof(char));

  if (safe_read(_log, kbuf, header->ksize) < 0)
    std::cout << "wotrget read key: " << strerror(errno) << std::endl;
  if (safe_read(_log, vbuf, header->vsize) < 0)
    std::cout << "wotrget read value: " << strerror(errno) << std::endl;

  *data = vbuf;
  *len = header->vsize;
  free(kbuf);

  return 0;
}

int Wotr::Flush() {
  return fsync(_log);
}
    
