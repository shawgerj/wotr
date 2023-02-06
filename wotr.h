#ifndef WOTR_H
#define WOTR_H

#include <string>
#include <mutex>

typedef struct {
  size_t ksize;
  size_t vsize;
} item_header;

class Wotr {
public:
  Wotr (const char* logname);

  int WotrWrite(std::string& logdata, int flush);
  int WotrGet(size_t offset, char** data, size_t* len);
  int Flush();
  int CloseAndDestroy();

private:
  std::string _logname;
  int _log; // fd
  off_t _offset;
  std::mutex _m;
};

#endif // WOTR_H
