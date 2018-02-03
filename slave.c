#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include "slave.h"

int main(int argc, char **argv)
{
	char query_word[32] = {};
	char file_path[1024] = {};
	unsigned int count = 0;
	char buffer[2048] = {};
	char temp[2048] = {};
	char unit[128] = {};
	FILE* file = NULL;
	struct mail_t mail, mails;
	int sysfs_fd, ret_val;

	//printf("This is slave.\n");
	while(1) {
		sysfs_fd = open("/sys/kernel/hw2/mailbox", O_RDONLY, S_IREAD);
		ret_val = receive_from_fd(sysfs_fd, &mail);
		close(sysfs_fd);

		sprintf(query_word, "%s", mail.data.query_word);
		sprintf(file_path, "%s", mail.file_path);
		//printf("[QUERY_WORD] %s ", query_word);
		//printf("[FILE_PATH] %s\n", file_path);

		file = fopen(file_path, "r");
		if(file == NULL) {
			puts("fail to open file.");
			return 1;
		}
		while(fgets(temp, 2048, file) != NULL) {
			strcat(buffer, temp);
		}
		int i = 0, j = 0;
		for (i=0; i<strlen(buffer); i++) {
			buffer[i]=tolower(buffer[i]);
		}

		for (i = 0; i < strlen(buffer); i++) {
			while (i < strlen(buffer) && !isspace(buffer[i]) && buffer[i]!='.'
			       &&buffer[i]!=','&&buffer[i]!='!'&&buffer[i]!='?') {
				unit[j++] = buffer[i++];
			}
			if (j != 0) {
				unit[j] = '\0';
				if (strcmp(unit, query_word) == 0)
					count++;
				j = 0;
			}
		}
		//printf("The total number of query word %s is %u.\n", query_word, count);
		mails.data.word_count = count;
		strcpy(mails.file_path, file_path);
		sysfs_fd = open("/sys/kernel/hw2/mailbox", O_WRONLY, S_IWRITE);
		send_to_fd(sysfs_fd, &mails);
		close(sysfs_fd);
		count = 0;
		memset(buffer,0,strlen(buffer));
	}
	return 0;
}

int send_to_fd(int sysfs_fd, struct mail_t *mail)
{
	char mailbuffer[1024] = {};
	sprintf(mailbuffer, "%u %s", mail->data.word_count, mail->file_path);
	//printf("slave sending... %s\n", mailbuffer);
	int ret_val = write(sysfs_fd, mailbuffer, sizeof(mailbuffer));
	if (ret_val == -1) {
		while(ret_val == -1) {
			//sleep(0.5);
			ret_val = write(sysfs_fd, mailbuffer, sizeof(mailbuffer));
		}
	} else {
		return ret_val;
	}
	return ret_val;
}

int receive_from_fd(int sysfs_fd, struct mail_t *mail)
{
	char mailbuffer[1024] = {};
	int ret_val = read(sysfs_fd, mailbuffer, sizeof(mailbuffer));
	if (ret_val == -1) {
		while(ret_val == -1) {
			//sleep(0.5);
			ret_val = read(sysfs_fd, mailbuffer, sizeof(mailbuffer));
		}
	} else {
	}
	//printf("slave receiving... %s\n", mailbuffer);
	sscanf(mailbuffer, "%s %s", mail->data.query_word, mail->file_path);
	return ret_val;
}
