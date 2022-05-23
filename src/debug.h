#ifndef DEFINE_DEBUG_ONCE
#define DEFINE_DEBUG_ONCE

#if DEBUG

#include <stdio.h>
#define debug(x, ...)                                                  \
  do {                                                                 \
    fprintf(stderr, "%s:%s(%u): " x "\n", __FILE__, __func__, __LINE__,\
            ##__VA_ARGS__);                                            \
  } while (0)

#else
#define debug(x, ...)
#endif

#endif
