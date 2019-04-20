#ifndef _UTILS_IO_H
#define _UTILS_IO_H

#include<unistd.h>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)

void syserr(char const *location);
void apperr(char const *location, char const *mssg);

int safe_read(int fd, void *buf, size_t len);

#endif
