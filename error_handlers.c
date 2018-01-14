#include "server.h"

void err_user_taken(int connfd, char *username) {
	char buf[MAX_BUF_SIZE];

	memset(buf, 0, MAX_BUF_SIZE);

	strcat(buf, ERROR_USERNAME);
	strcat(buf, "USER NAME TAKEN ");
	strcat(buf, username);
	strcat(buf, END_PROTOCOL);

	send_to_client(connfd, buf);
}

void err_bad_password(int connfd) {
	char buf[MAX_BUF_SIZE];

	memset(buf, 0, MAX_BUF_SIZE);

	strcat(buf, ERROR_PASSWORD);
	strcat(buf, "BAD PASSWORD");
	strcat(buf, END_PROTOCOL);

	send_to_client(connfd, buf);
}

void err_user_not_avail(int connfd) {
	char buf[MAX_BUF_SIZE];

	memset(buf, 0, MAX_BUF_SIZE);

	strcat(buf, ERROR_NOT_AVAIL);
	strcat(buf, "USER NOT AVAILABLE");
	strcat(buf, END_PROTOCOL);

	send_to_client(connfd, buf);
}

void err_server(int connfd) {
	char buf[MAX_BUF_SIZE];

	memset(buf, 0, MAX_BUF_SIZE);

	strcat(buf, ERROR_SERVER);
	strcat(buf, "INTERNAL SERVER ERROR");
	strcat(buf, END_PROTOCOL);

	send_to_client(connfd, buf);
}