#ifndef C_WOTR_INCLUDE_CWRAPPER_H_
#define C_WOTR_INCLUDE_CWRAPPER_H_

#pragma once
#define WOTR_LIBRARY_API

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h> // needed on my fedora system to compile ssize_t?

typedef struct wotr_t wotr_t;
typedef struct wotr_iter_t wotr_iter_t;
typedef struct entry_t entry_t;

extern WOTR_LIBRARY_API
wotr_t* wotr_open(const char* logfile, char** errptr);
    
extern WOTR_LIBRARY_API
ssize_t wotr_write(wotr_t* w, const char* logdata, size_t len);

extern WOTR_LIBRARY_API
ssize_t wotr_write_entry(wotr_t* w, const char* key, size_t key_size,
			 const char* value, size_t value_size,
			 uint32_t cfid);
  
extern WOTR_LIBRARY_API
int wotr_get(wotr_t* w, size_t offset, char** data, size_t len);

extern WOTR_LIBRARY_API
ssize_t wotr_head(wotr_t* w);
    
extern WOTR_LIBRARY_API
int wotr_sync(wotr_t* w);

extern WOTR_LIBRARY_API
int wotr_deallocate(wotr_t* w, size_t start, size_t length);

extern WOTR_LIBRARY_API
void wotr_close(wotr_t* w);

extern WOTR_LIBRARY_API
wotr_iter_t* wotr_iter_init(wotr_t* w, char** errptr);

extern WOTR_LIBRARY_API
void wotr_iter_seek(wotr_iter_t* wi, size_t offset);

extern WOTR_LIBRARY_API
void wotr_iter_next(wotr_iter_t* wi);

extern WOTR_LIBRARY_API
int wotr_iter_valid(wotr_iter_t* wi);

extern WOTR_LIBRARY_API
char* wotr_iter_key(wotr_iter_t* wi);

extern WOTR_LIBRARY_API
char* wotr_iter_value(wotr_iter_t* wi);

extern WOTR_LIBRARY_API
size_t wotr_iter_key_size(wotr_iter_t* wi);

extern WOTR_LIBRARY_API
size_t wotr_iter_value_size(wotr_iter_t* wi);

extern WOTR_LIBRARY_API
size_t wotr_iter_position(wotr_iter_t* wi);

extern WOTR_LIBRARY_API
int wotr_iter_get_cfid(wotr_iter_t* wi);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* C_WOTR_INCLUDE_CWRAPPER_H_ */
