#ifndef SF_WRITE
#define SF_WRITE

#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>

void sfwrite(pthread_mutex_t *, FILE *, char  *, ...);

#endif
