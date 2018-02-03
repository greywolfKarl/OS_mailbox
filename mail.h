#ifndef MAIL_H
#define MAIL_H

#include "module/def.h"

struct mail_t {
	union {
		char query_word[32];
		unsigned int word_count;
	} data;
	char file_path[4096];
};

int send_to_fd(int sysfs_fd, struct mail_t *mail);
int receive_from_fd(int sysfs_fd, struct mail_t *mail);

#endif
