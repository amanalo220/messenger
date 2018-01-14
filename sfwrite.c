#include "sfwrite.h"

void sfwrite(pthread_mutex_t *lock, FILE *stream, char *fmt, ...) {
	va_list valist;
	
	pthread_mutex_lock(lock);
	va_start(valist, fmt);

	vfprintf(stream, fmt, valist);

	va_end(valist);
	pthread_mutex_unlock(lock);
}