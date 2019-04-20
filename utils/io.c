#include<errno.h>
#include<unistd.h>
#include<stdio.h>
#include<string.h>

#include "io.h"

extern int errno;

void syserr(char const *location) {
	fprintf(stderr, "%s - %s\n", location, strerror(errno));
}

void apperr(char const *location, char const *mssg) {
	fprintf(stderr, "%s - %s\n", location, mssg);
}

int safe_read(int fd, void *buf, size_t len) {
	ssize_t ret, orig = len;

	while (len != 0 && (ret = read(fd, buf, len)) != 0) {
		if (ret == -1) {
			if (errno == EINTR) continue;
			return -1;
		}

		len -= ret;
		buf += ret;
	}

	return orig - len;
}
