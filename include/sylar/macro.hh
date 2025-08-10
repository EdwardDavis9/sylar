#ifndef __SYLAR_MACRO_H__
#define __SYLAR_MACRO_H__

#include "sylar/util.hh"
#include <assert.h>
#include <string>

#if defined __GNUC__ || defined __llvm__
#define SYLAR_LICKLY(x) __builtin_expect(!!(x), 1)
#define SYLAR_UNLICKLY(x) __builtin_expect(!!(x), 0)
#else
#define SYLAR_LICKLY(x) (x)
#define SYLAR_UNLICKLY(x) (x)
#endif


#define SYLAR_ASSERT(x)                                                        \
  if (SYLAR_UNLICKLY(!(x))) {                                                  \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())                                          \
        << "ASSERTION: " #x << "\nbacktrace:\n"                                \
        << sylar::BacktraceToString(100, 2, "    ");                           \
    assert(x);                                                                 \
  }

// 这里隐藏前面两层不重要的内容
#define SYLAR_ASSERT2(x, w)                                                    \
  if (SYLAR_UNLICKLY(!(x))) {                                                  \
    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT())                                          \
        << "ASSERTION: " #x << "\n"                                            \
        << w << "\nbacktrace:\n"                                               \
        << sylar::BacktraceToString(100, 2, "    ");                           \
    assert(x);                                                                 \
  }

#endif // __SYLAR_MACRO_H__
