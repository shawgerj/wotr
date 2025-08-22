#ifndef WOTR_H
#define WOTR_H

#include <cstdint>
#include <string>
#include <mutex>
#include <unordered_map>
#include <fstream>

#define STAT_BUF_SIZE (4096)

// used in rocksdb
struct wotr_ref {
  size_t offset;
  size_t len;
};
  
typedef struct {
  size_t ksize;
  size_t vsize;
  uint32_t cfid;
} item_header;

struct kv_entry_info {
  uint32_t cfid;
  size_t ksize;
  size_t vsize;
  size_t key_offset;
  size_t value_offset;
  size_t offset; // offset of the header
  size_t size; // entry size. use to get offset of next entry if iterating log
};

typedef kv_entry_info Entry;

class Wotr {
public:
  Wotr (const char* logname);

  ssize_t WotrWrite(std::string& logdata);
  // an Entry-aware right function for garbage collection in tikv
  // really just here to simplify my rust code... 
  int WotrGet(size_t offset, char** data, size_t len);
  ssize_t Head();
  int Sync();
  int Deallocate(size_t start, size_t length);

  int StartupRecovery(std::string path, size_t logstart);

  int CloseAndDestroy();

  friend class WotrIter;

private:
  std::string _logname;
  int _log; // fd
  //  int _db_counter;
  ssize_t _offset;
  std::mutex _lock;

  int check_bounds(size_t offset); 
  int safe_write(int fd, const char* data, size_t size);
  int get_entry(size_t offset, Entry* entry);

  // maybe useful later... these are set up in Wotr::Wotr()
  int _statslog; // fd
  char* _statsstart;
  char* _statsptr;
  size_t _inception; // time in ns start of program
};

class WotrIter {
public:
  WotrIter(Wotr& wotr);
  ~WotrIter();

  void seek(size_t offset);
  void next();
  bool valid();

  char* key();
  char* value();
  size_t key_size();
  size_t value_size();
  size_t position();

  uint32_t GetCfID();

private:
  Wotr& w;
  Entry curr_; // header of the current entry
  char* key_;
  char* value_;
  size_t offset_;
  bool valid_;

  int load_data();
};

#endif // WOTR_H
