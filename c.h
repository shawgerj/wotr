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

typedef struct wotr_t wotr_t;

extern WOTR_LIBRARY_API wotr_t* wotr_open(const char* logfile);
extern WOTR_LIBRARY_API int wotr_write(wotr_t* w, const char* logdata, size_t len);
extern WOTR_LIBRARY_API int wotr_get(wotr_t* w, size_t offset, char** data, size_t* len);
extern WOTR_LIBRARY_API void wotr_close(wotr_t* w);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* C_WOTR_INCLUDE_CWRAPPER_H_ */
