#include "server.h"

int wolfie_protocol_handler(int connfd) {
	int i;
	char buf[MAX_INPUT];

	memset(buf, 0, MAX_INPUT);

	if((i = read(connfd, buf, MAX_INPUT)) > 0) {
		if(strcmp(buf, WOLFIE_PROTOCOL) == 0) {
			send_to_client(connfd, EIFLOW_PROTOCOL);
		} else {
			sfwrite(&lock, stderr, "%s\n", buf);
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Wolfie Protocol Failed" ANSI_DEFAULT_COLOR "\n");
			err_server(connfd);
			return -1;
		}
	} else {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Wolfie Protocol Read Failed" ANSI_DEFAULT_COLOR "\n");
		err_server(connfd);
		return -1;
	}

	if(v) {
		verbose(buf, VERBOSE_IN);
	}

	return 1;
}

int username_check(char *username) {
	
	struct account *a;

	if(account_head == NULL) {
		return 1;
	}

	a = account_head;

	while(a != NULL) {
		if(strcmp(username, a->user_name) == 0) {
			return -1;
		}

		a = a->next;
	}
 
	return 1;
}

int password_check(char *pw) {

	char *c = pw;
	bool upper = false;
	bool symbol = false;
	bool num = false;

	if(strlen(pw) < 5) {			//password length >= 5
		return -1;
	}

	while(*c != '\0') {
		// sfwrite(&lock, stdout, "%c\n", *c);
		if(*c >= 65 && *c <= 90) {			//1 uppercase
			upper = true;
		} else if((*c >= 33 && *c <= 47) || (*c >= 58 && *c <= 64) || (*c >= 91 && *c <= 96) || (*c >= 123 && *c <= 126)) {		//1 symbol
			symbol = true;
		} else if(*c >= 48 && *c <= 57) {			//1 number
			num = true;
		}

		c++;
	}

	if(!upper || !symbol || !num) {
		return -1;
	}


	return 1;
}

int login_protocol(int connfd) {

	int i;
	char input[MAX_INPUT], buf[MAX_BUF_SIZE], username[MAX_USER_NAME_SIZE], pw[MAX_PASSWORD_SIZE];
	char *token, *in_ptr;
	char **ptr;
	struct client *c = client_head;
	struct account *a = account_head;
	bool finished = false;

	memset(input, 0, MAX_INPUT);
	memset(buf, 0, MAX_BUF_SIZE);
	memset(username, 0, MAX_USER_NAME_SIZE);
	memset(pw, 0, MAX_PASSWORD_SIZE);

	if((i = read(connfd, input, MAX_INPUT)) < 0) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Failed to read username" ANSI_DEFAULT_COLOR "\n");
		err_server(connfd);
		return -1;
	}

	if(v) {
		verbose(input, VERBOSE_IN);
	}

	in_ptr = input;
	ptr = &in_ptr;

	token = strtok_r(input, " ", ptr);

	if(strcmp(token, CLIENT_LOGIN) == 0) {
		
		token = strtok_r(NULL, " ", ptr);

		while(c != NULL) {
			if(strcmp(token, c->user_name) == 0) {
				sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Username taken" ANSI_DEFAULT_COLOR "\n");
				err_user_taken(connfd, token);
				return -1;
			} else {
				c = c->next;
			}
		}

		fprintf(stdout, "got here\n");

		while(a != NULL) {
			if(strcmp(token, a->user_name) == 0) {
				finished = true;
				break;
			} else {
				a = a->next;
			}
		}

		if(finished) {
			strcat(buf, SERVER_AUTHENTICATE);
			strcat(buf, " ");
			strcat(buf, token);
			strcat(buf, END_PROTOCOL);

			send_to_client(connfd, buf);

			if(add_client(connfd, token) < 0) {
				err_server(connfd);
				return -1;
			}

			read(connfd, input, MAX_INPUT);

			if(v) {
				verbose(input, VERBOSE_IN);
			}

			in_ptr = input;
			ptr = &in_ptr;

			token = strtok_r(input, " ", ptr);

			if(strcmp(token, CLIENT_PASSWORD) == 0) {
				token = strtok_r(NULL, " ", ptr);

				if(strcmp(token, a->password) == 0) {
					memset(buf, 0, MAX_BUF_SIZE);

					strcat(buf, SERVER_PASSWORD);
					strcat(buf, END_PROTOCOL);

					send_to_client(connfd, buf);

					memset(buf, 0, MAX_BUF_SIZE);

					strcat(buf, SERVER_LOGIN);
					strcat(buf, " ");
					strcat(buf, a->user_name);
					strcat(buf, END_PROTOCOL);

					send_to_client(connfd, buf);

					memset(buf, 0, MAX_BUF_SIZE);

					strcat(buf, SERVER_MSG);
					strcat(buf, " ");
					strcat(buf, msg_buf);
					strcat(buf, END_PROTOCOL);

					send_to_client(connfd, buf);
				} else {
					sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "incorrect password" ANSI_DEFAULT_COLOR "\n");
					err_bad_password(connfd);
				}
			}

		} else {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Unknown username" ANSI_DEFAULT_COLOR "\n");
			err_user_not_avail(connfd);
		}

		// sfwrite(&lock, stdout, "got to send client\n");

	} else if(strcmp(token, CLIENT_NEW_USER) == 0) {			//new user

		token = strtok_r(NULL, " ", ptr);

		if(username_check(token) < 0) {							//username protocol
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "User name taken" ANSI_DEFAULT_COLOR "\n");
			err_user_taken(connfd, token);
			return -1;
		}

		strcat(buf, SERVER_NEW_USER);
		strcat(buf, " ");
		strcat(buf, token);
		strcat(buf, END_PROTOCOL);

		send_to_client(connfd, buf);

		strcat(username,token);

		memset(input, 0, MAX_INPUT);

		if(read(connfd, input, MAX_INPUT) < 0) {				//password protocol
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error reading password protocol" ANSI_DEFAULT_COLOR "\n");
			err_server(connfd);
			return -1;
		}

		if(v) {
			verbose(input, VERBOSE_IN);
		}

		in_ptr = input;
		ptr = &in_ptr;

		token = strtok_r(input, " ", ptr);

		if(strcmp(token, CLIENT_NEW_PASS) == 0) {
			token = strtok_r(NULL, " ", ptr);

			sfwrite(&lock, stdout, "%s\n", token);

			if(password_check(token) < 0) {
				sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Invalid password" ANSI_DEFAULT_COLOR "\n");
				err_bad_password(connfd);
				return -1;
			}

			strcat(pw, token);

			memset(buf, 0, MAX_BUF_SIZE);

			strcat(buf, SERVER_NEW_PASS);
			strcat(buf, END_PROTOCOL);

			send_to_client(connfd, buf);

			add_client(connfd, username);

			create_account(username, pw);

			memset(buf, 0, MAX_BUF_SIZE);

			strcat(buf, "HI ");
			strcat(buf, username);
			strcat(buf, END_PROTOCOL);

			send_to_client(connfd, buf);

			memset(buf, 0, MAX_BUF_SIZE);

			strcat(buf, SERVER_MSG);
			strcat(buf, " ");
			strcat(buf, msg_buf);
			strcat(buf, END_PROTOCOL);

			send_to_client(connfd, buf);

		}

	} else {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Username Protocol Failed" ANSI_DEFAULT_COLOR "\n");
		err_server(connfd);
		return -1;
	}

	return 1;
}

void client_logout(int connfd) {
	struct client *cp;
	cp = client_head;
	char buf[MAX_USER_NAME_SIZE], logout[MAX_BUF_SIZE];

	memset(logout, 0, MAX_BUF_SIZE);
	
	strcat(logout, CLIENT_LOGOUT);
	strcat(logout, END_PROTOCOL);

	send_to_client(connfd, logout);

	while(cp->fd != connfd) {
		cp = cp->next;
	}

	printf("connfd: %d\nfd: %d\n", connfd, cp->fd);

	memset(buf, 0, MAX_USER_NAME_SIZE);
	strcat(buf, cp->user_name);
	
	memset(logout, 0, MAX_BUF_SIZE);

	strcat(logout, UOFF);
	strcat(logout, " ");
	strcat(logout, buf);
	strcat(logout, END_PROTOCOL);

	remove_client(cp);

	cp = client_head;

	while(cp != NULL) {
		send_to_client(cp->fd, logout);
		cp = cp->next;
	}

	sfwrite(&lock, stdout, "User %s has logged out\n", buf);

}

void client_listu(int connfd) {
	char buf[MAX_BUF_SIZE];
	struct client *c;

	memset(buf, 0, MAX_BUF_SIZE);

	strcat(buf, SERVER_UTSIL);
	strcat(buf, " ");
	// char_count += 6;

	pthread_mutex_lock(&lock);

	c = client_head;

	while(c != NULL) {
		strcat(buf, c->user_name);
		strcat(buf, USER_PROTOCOL);

		// char_count += strlen(c->user_name);
		// char_count += 3;

		c = c->next;
	}

	pthread_mutex_unlock(&lock);

	strcat(buf, END_PROTOCOL);

	send_to_client(connfd, buf);
	// write(connfd, buf, char_count + 1);
}

void client_time(int connfd) {
	struct client *c = client_head;

	while(c->fd != connfd) {
		c = c->next;
	}

	double current_time = time(NULL);
	double diff_time = current_time - c->start_time;

	char times[50];
	sprintf(times, "%f", diff_time);

	char buf[MAX_BUF_SIZE];
	memset(buf, 0, MAX_BUF_SIZE);
	strcat(buf, "EMIT ");
	strcat(buf, times);
	strcat(buf, " \r\n\r\n");

	send_to_client(connfd, buf);
}

void client_msg(int connfd, char *msg) {
	char *token, copy[MAX_INPUT];
	int rfd;

	strcpy(copy, msg);

	token = strtok(msg, " ");				//MSG
	token = strtok(NULL, " ");				//recipient username

	rfd = findFD(token);

	if(rfd < 0) {
		err_server(connfd);
	}

	send_to_client(connfd, copy);
	send_to_client(rfd, copy);
	
}