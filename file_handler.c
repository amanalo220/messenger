#include "server.h"

FILE *init_user_file(char *file_name) {
	FILE *user_info_file;

	user_info_file = fopen(file_name, "a+");

	return user_info_file;
}

char *get_user_info(FILE *file, int user_num) {
	char *line;
	int count = 0;
	ssize_t i;

	while((i = getline(&line, 0, file)) != -1 || count == user_num) {
		count ++;
	}

	if(i == -1) {
		sfwrite(&lock, stderr, "Error getting user info from file: %s\n", strerror(errno));
		return NULL;
	}

	return line;
}

int add_user_info(FILE *file, char *username, char *password) {
	char buf[MAX_BUF_SIZE];
	int fd;

	memset(buf, 0, MAX_BUF_SIZE);

	strcat(buf, username);
	strcat(buf, " ");
	strcat(buf, password);
	strcat(buf, "\n");					//username password\n

	fd = fileno(file);
	flock(fd, LOCK_EX);

	if(!(fputs(buf, file) > 0)) {
		return -1;
	}

	flock(fd, LOCK_UN);

	return 1;
}

void retrieve_accounts(FILE *acct_file) {
	char *lineptr, *token, username[MAX_BUF_SIZE], password[MAX_BUF_SIZE];
	size_t length = MAX_INPUT;
	int fd;

	fd = fileno(acct_file);
	flock(fd, LOCK_SH);

	while(getline(&lineptr, &length, acct_file) > 0) {

		token = strtok(lineptr, " ");
		strcpy(username, token);				//username

		token = strtok(NULL, "\n");
		strcpy(password, token);				//password

		create_account(username, password);
	}

	flock(fd, LOCK_UN);
}

void close_user_file(FILE *file) {
	fclose(file);
}