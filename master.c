#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include "master.h"

int count = 0;

void directoria(char* directory, struct mail_t *mail, struct mail_t *mailr)
{
	int sysfs_fd, ret;
	struct dirent *name;
	DIR *fldptr;
	DIR *direct = opendir(directory);
	char *flag = "";
	char filename[1024] = {};

	if(direct == NULL) {
		puts("directory not exist.");
		return 1;
	}
	while(name = readdir(direct)) {
		flag = name->d_name;
		if(flag[0] != '.') {	//ignore
			snprintf(filename, sizeof(filename), "%s/%s", directory, flag);
			if(fldptr = opendir(filename)) {	//is a folder
				closedir(fldptr);
				directoria(filename, mail, mailr);
			} else {	//not a folder
				sprintf(mail->file_path, "%s",filename);
				sysfs_fd = open("/sys/kernel/hw2/mailbox", O_WRONLY, S_IWRITE);
				ret = send_to_fd(sysfs_fd, mail);
				close(sysfs_fd);
				if(ret == -1) {
					sysfs_fd = open("/sys/kernel/hw2/mailbox", O_RDONLY, S_IREAD);
					ret = receive_from_fd(sysfs_fd, mailr);
					close(sysfs_fd);
					while(ret == -1) {
						sysfs_fd = open("/sys/kernel/hw2/mailbox", O_RDONLY, S_IREAD);
						ret = receive_from_fd(sysfs_fd, mailr);
						close(sysfs_fd);
					}
					sysfs_fd = open("/sys/kernel/hw2/mailbox", O_WRONLY, S_IWRITE);
					ret = send_to_fd(sysfs_fd, mail);
					close(sysfs_fd);
				}
			}
		}
	}
	close(direct);
}

int main(int argc, char **argv)
{
	//-q QUERY_WORD -d DIRECTORY -s K
	char arg;
	char query_word[32] = {};
	char directory[1024] = {};
	char filename[1024] = {};
	char slaves[5] = {};
	int k = 1, i;
	DIR* direct = NULL;
	struct dirent* name;
	char *flag = "";
	struct mail_t mail, mailr;
	char *args[] = {"/home/user/hw2_mailbox/slave", NULL};
	int sysfs_fd, ret;

	while ((arg = getopt(argc, argv, "q:d:s:")) != EOF) {
		switch (arg) {
		case 'q':
			sprintf(query_word, "%s", optarg);
			break;
		case 'd':
			sprintf(directory, "%s", optarg);
			break;
		case 's':
			sprintf(slaves, "%s", optarg);
			k = atoi(slaves);
			if(!k)	//if not num
				k = 1;
			break;
		default:
			printf("Illegal argument %s please try again.\n", optarg);
			return 1;
		}
	}
	printf("[QUERY_WORD] %s ", query_word);
	printf("[DIRECTORY] %s ", directory);
	printf("[K] %d\n", k);

	pid_t pids[k];
	for(i=0; i<k; i++) {
		pid_t pid = fork();
		if (pid == 0) {	// child
			printf("# slave %d [PID] %d\n", i+1, getpid());
			execv("/home/user/hw2_mailbox/slave", args);
			exit(0);
		} else if (pid < 0) {
			puts("fork failed.");
			return 1;
		} else {	//parent
			pids[i] = pid;
		}
	}

	sprintf(mail.data.query_word, "%s", query_word);
	directoria(directory, &mail, &mailr);
	//puts("over.");
	while(count > 0) {
		sysfs_fd = open("/sys/kernel/hw2/mailbox", O_RDONLY, S_IREAD);
		ret = receive_from_fd(sysfs_fd, &mailr);
		close(sysfs_fd);
	}

	for(i=0; i<k; i++) {
		kill(pids[i], 15);	//SIGTERM
		printf("slave %d killed\n", pids[i]);
		waitpid(pids[i], NULL, WNOHANG);
	}
	return 0;
}

int send_to_fd(int sysfs_fd, struct mail_t *mail)
{
	char mailbuffer[1024] = {};
	sprintf(mailbuffer, "%s %s", mail->data.query_word, mail->file_path);
	int ret_val = write(sysfs_fd, mailbuffer, sizeof(mailbuffer));
	if (ret_val == -1)
		return ret_val;
	else {
		//printf("master sending... %s\n", mailbuffer);
		count++;
		//printf("count: %d\n", count);
	}
	return count;
}

int receive_from_fd(int sysfs_fd, struct mail_t *mail)
{
	char mailbuffer[1024] = {};
	int ret_val = read(sysfs_fd, mailbuffer, sizeof(mailbuffer));
	if (ret_val == -1) {
		return ret_val;
	} else {
		//printf("master receiving... %s\n", mailbuffer);
		count--;
		//printf("count: %d\n", count);
		sscanf(mailbuffer, "%u %s", &mail->data.word_count, mail->file_path);
		printf("[WORD_COUNT] %u ", mail->data.word_count);
		printf("[FILE_PATH] %s\n", mail->file_path);
	}
	return count;
}
