#include "c.h"
#include "wotr.h"
#include <string>
#include <system_error>
#include <string.h>

extern "C" {
  struct wotr_t      { Wotr* rep; };
  struct wotr_iter_t { WotrIter* rep; };
  struct entry_t     { Entry* rep; };

  wotr_t* wotr_open(const char* logfile, char** errptr) {
    Wotr* w;
    try {
      w = new Wotr(logfile);
    } catch (const std::system_error& e) {
      *errptr = strdup(e.what());
      return nullptr;
    }

    wotr_t* res = new wotr_t;
    res->rep = w;
    return res;
  }

  ssize_t wotr_write(wotr_t* w, const char* logdata, size_t len) {
    std::string data(logdata, len);
    return w->rep->WotrWrite(data);
  }
  
  int wotr_get(wotr_t* w, size_t offset, char** data, size_t* len) {
    return w->rep->WotrGet(offset, data, len);
  }

  int wotr_p_get(wotr_t* w, size_t offset, char** data, size_t len) {
    return w->rep->WotrPGet(offset, data, len);
  }

  ssize_t wotr_head(wotr_t* w) {
    return w->rep->Head();
  }

  int wotr_sync(wotr_t* w) {
    return w->rep->Sync();
  }

  void wotr_close(wotr_t* w) {
    delete w->rep;
    delete w;
  }

  wotr_iter_t* wotr_iter_init(wotr_t* w, char** errptr) {
    WotrIter* wi;

    try {
      wi = new WotrIter(*(w->rep));
    } catch (const std::system_error& e) {
      *errptr = strdup(e.what());
      return nullptr;
    }

    wotr_iter_t* res = new wotr_iter_t;
    res->rep = wi;
    return res;
  }

  int wotr_iter_read(wotr_iter_t* wi, entry_t* e) {
    return wi->rep->read(e->rep);
  }

  void wotr_iter_set_offset(wotr_iter_t* wi, size_t offset) {
    wi->rep->set_offset(offset);
  }

  void wotr_iter_next(wotr_iter_t* wi) {
    wi->rep->next();
  }

  char* wotr_iter_read_key(wotr_iter_t* wi) {
    return wi->rep->read_key();
  }

  char* wotr_iter_read_value(wotr_iter_t* wi) {
    return wi->rep->read_value();
  }

  int wotr_iter_get_cfid(wotr_iter_t* wi) {
    return wi->rep->GetCfID();
  }

}
