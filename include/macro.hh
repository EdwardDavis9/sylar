#ifndef __SYLAR_MACRO_H__
#define __SYLAR_MACRO_H__

#include "util.hh"
#include <assert.h>
#include <string>

#define SYLAR_ASSERT(x)                                                        \
  if (!(x)) {                                                                  \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())                                          \
        << "ASSERTION: " #x << "\nbacktrace:\n"                                \
        << sylar::BacktraceToString(100, 2, "    ");                           \
    assert(x);                                                                 \
  }

// 这里隐藏前面两层不重要的内容
#define SYLAR_ASSERT2(x, w)                                                    \
  if (!(x)) {                                                                  \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())                                          \
        << "ASSERTION: " #x << "\n"                                            \
        << w << "\nbacktrace:\n"                                               \
        << sylar::BacktraceToString(100, 2, "    ");                           \
    assert(x);                                                                 \
  }

#endif // __SYLAR_MACRO_H__
