#include "server.h"

void print_help() {
	printf("./server [-h|-v] PORT_NUMBER MOTD [ACCOUNTS_FILE]\n");
	printf("-h            Displays help menu & returns EXIT_SUCCESS.\n");
	printf("-v            Verbose print all incoming and outgoing protocol verbs & content.\n");
	printf("PORT_NUMBER   Port number to listen on.\n");
	printf("MOTD          Message to display to the client when they connect.\n");
	printf("ACCOUNTS_FILE File containing username and password data to be loaded upon execution\n\n");
}

int open_listenfd(char *port) {
	struct addrinfo hints, *listp, *p;
	int listenfd, i, opt = 1;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
	hints.ai_flags |= AI_NUMERICSERV;

	if((i = getaddrinfo(NULL, port, &hints, &listp)) != 0) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "getaddrinfo: %s" ANSI_DEFAULT_COLOR "\n", gai_strerror(i));
		return -1;
	}

	for(p = listp; p; p = p->ai_next) {
		
		if((listenfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0) {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Socket not made: %s" ANSI_DEFAULT_COLOR "\n", strerror(errno));
			continue;
		}

		sfwrite(&lock, stdout, "listenfd: %d\n", listenfd);

		if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&opt, sizeof(int)) < 0) {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error in setsockopt: %s" ANSI_DEFAULT_COLOR "\n", strerror(errno));
		} 

		if(bind(listenfd, p->ai_addr, p->ai_addrlen) == 0) {
			sfwrite(&lock, stdout, "Bind success\n");
			break;
		} else {
			sfwrite(&lock, stdout, ANSI_ERRORS_COLOR "Bind Error: %s" ANSI_DEFAULT_COLOR "\n", strerror(errno));
		}

		close(listenfd);
	}

	freeaddrinfo(listp);

	if(!p) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error: no p in open_listenfd" ANSI_DEFAULT_COLOR "\n");
		return -1;
	}

	if(listen(listenfd, 128) < 0) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error in listen: %s" ANSI_DEFAULT_COLOR "\n", strerror(errno));
		close(listenfd);
		return -1;
	}

	return listenfd;
}

int add_client(int connfd, char *username) {
	struct client *c = (struct client *)calloc(1, sizeof(struct client));
	
	c->fd = connfd;
	c->next = NULL;
	c->buf_ptr = c->buf;
	c->start_time = time(NULL);

	if(strlen(username) <= MAX_USER_NAME_SIZE) {
		strcpy(c->user_name, username);
	} else {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Username too long" ANSI_DEFAULT_COLOR "\n");
		return -1;
	}

	if(client_head == NULL) {						//first client
		client_head = c;
		c->prev = NULL;
	} else {										//not first client
		struct client *c2 = client_head;

		while(c2->next != NULL) {
			c2 = c2->next;
		}

		c2->next = c;
		c->prev = c2;
	}
	
	FD_SET(connfd, &read_set);
	num_of_fd++;

	sfwrite(&lock, stdout, "Connection made with client: %s\n", c->user_name);

	return 1;
}

void remove_client(struct client *c) {
	struct client *c2;

	if(c == client_head) {
		if(c->next != NULL) {
			c->next->prev = NULL;
		}

		client_head = c->next;
	} else {
		c2 = c->prev;
		c2->next = c->next;
		
		if(c->next != NULL) {
			c->next->prev = c2;
		}
	}

	FD_CLR(c->fd, &read_set);

	close(c->fd);
	free(c);
}

void parse_scommand() {
	int n;
	char buf[MAX_INPUT];

	if((n = read(STDIN_FILENO, buf, MAX_INPUT)) > 0) {
		if(strcmp(buf, "/users\n") == 0) {
			users_cmd_handler();
			// sfwrite(&lock, stdout, "In parse_scommand\n%s\n", buf);
		} else if(strcmp(buf, "/help\n") == 0) {
			help_cmd_handler();
		} else if(strcmp(buf, "/shutdown\n") == 0) {
			shutdown_cmd_handler();
			// sfwrite(&lock, stdout, "In parse_scommand\n%s\n", buf);
		} else if(strcmp(buf, "/accts\n") == 0) {
			accounts_cmd_handler();
		} else {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Command %s not available" ANSI_DEFAULT_COLOR "\n\n", buf);
			help_cmd_handler();
		}

	} else if(n < 0) {

		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error on read from stdin: %s" ANSI_DEFAULT_COLOR "\n", strerror(errno));

	} else {

		sfwrite(&lock, stdout, ANSI_ERRORS_COLOR "Read no bytes from stdin." ANSI_DEFAULT_COLOR "\n");

	}

	if(v) {
		verbose(buf, VERBOSE_IN);
	}
}

void send_to_client(int connfd, char *msg) {
	int n;

	if((n = write(connfd, msg, strlen(msg))) < 0) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error writing to client: %s" ANSI_DEFAULT_COLOR "\n", strerror(errno));
	}

	if(v) {
		verbose(msg, VERBOSE_OUT);
	}

}

void parse_ccommands(int connfd) {
	char buf[MAX_INPUT];
	char copy[MAX_INPUT];
	int i;

	if((i = read(connfd, buf, MAX_INPUT)) < 0) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error in reading client command" ANSI_DEFAULT_COLOR "\n");
		// sfwrite(&lock, stderr, "%s" "\n", strerror(errno));
		return;
	} else if(i == 0) {
		return;
	}

	if(v) {
		verbose(buf, VERBOSE_IN);
	}

	strcpy(copy, buf);
	
	char *verb = strtok(buf, " ");

	if(strcmp(verb, CLIENT_LOGOUT) == 0) {
		client_logout(connfd);
	} else if(strcmp(verb, CLIENT_LISTU) == 0){
		// printf("got here\n");	
		client_listu(connfd);
	} else if(strcmp(verb, CLIENT_TIME) == 0) {
		client_time(connfd);
	} else if(strcmp(verb, MSG) == 0) {
		// printf("got a message\n");
		client_msg(connfd, copy);
	} else {
		send_to_client(connfd, "Unknown client command\n");
	}
}

void *login_thread(void *vargp) {
	int connfd = *((int*)vargp);
	int i;
	
	pthread_detach(pthread_self());
	
	if((i = wolfie_protocol_handler(connfd)) < 0) {
		free(vargp);
		close(connfd);

		return NULL;
	} else {
		if(login_protocol(connfd) < 0) {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Login Failed" ANSI_DEFAULT_COLOR "\n");
			free(vargp);
			close(connfd);

			return NULL;
		} else {

			if(!com_thread_set) {
				pthread_t tid;
				com_thread_set = true;

				pthread_create(&tid, NULL, com_thread, NULL);
			}

		}
	}
	printf("logged in\n");
	free(vargp);

	return NULL;
}

void *new_login_thread(void *vargp) {
	pthread_mutex_t *q_lock = ((pthread_mutex_t *) vargp);
	int connfd, i;
	bool suc;

	pthread_detach(pthread_self());

	while(!sd) {
		sem_wait(&items_sem);
		suc = false;

		pthread_mutex_lock(q_lock);
		connfd = login_dequeue();
		pthread_mutex_unlock(q_lock);

		if((i = wolfie_protocol_handler(connfd)) < 0) {
			close(connfd);
		} else {
			if(login_protocol(connfd) < 0) {
				sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Login Failed" ANSI_DEFAULT_COLOR "\n");
				close(connfd);
			} else {
				suc = true;
				
				if(!com_thread_set) {
					pthread_t tid;
					com_thread_set = true;

					pthread_create(&tid, NULL, com_thread, NULL);
				}

			}
		}

		if(suc) {
			printf("logged in\n");	
		}
		
	}
	
	free(vargp);
	return NULL;
}

void *com_thread(void *vargp) {
	struct client *cp = client_head;
	pthread_detach(pthread_self());
	
	sfwrite(&lock, stdout, "%d\n", cp->fd);
	while(1) {

		if(sd) {
			break;				//terminate thread
		} else {
			cp = client_head;
			ready_set = read_set;

			pselect(num_of_fd, &ready_set, NULL, NULL, NULL, NULL);
			
			// sfwrite(&lock, stdout, "In Com thread\n");

			//cycle through file descriptors in user list
			while(cp != NULL) {
				if(FD_ISSET(cp->fd, &ready_set)) {
					//sfwrite(&lock, stdout, "Detected write, fd: %d\n", cp->fd);
					parse_ccommands(cp->fd);
					break;
				}
				cp = cp->next;
			}

		}
	}

	return NULL;
}

void create_account(char *username, char *password) {
	struct account *a = (struct account*)calloc(1, sizeof(struct account));

	strcpy(a->user_name, username);
	strcpy(a->password, password);

	add_user_info(user_file, username, password);

	a->next = NULL;

	if(account_head == NULL) {
		account_head = a;
		a->prev = NULL;
	} else {

		struct account *a2 = account_head;

		while(a2->next != NULL) {
			a2 = a2->next;
		}

		a2->next = a;
		a->prev = a2;
	}
}

void verbose(char *verb, int i) {
	char buf[MAX_BUF_SIZE];

	memset(buf, 0, MAX_BUF_SIZE);

	if(i == VERBOSE_IN) {
		strcat(buf, IN_STRING);
	} else if(i == VERBOSE_OUT) {
		strcat(buf, OUT_STRING);
	}

	strcat(buf, verb);

	sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "%s" ANSI_DEFAULT_COLOR "\n", buf);
}

int findFD(char *username) {
	struct client *client = client_head;

	while(client != NULL && strcmp(client->user_name, username) != 0) {
		client = client->next;
	}

	// printf("segfault?\n");

	if(client == NULL) {
		return -1;
	}

	return client->fd;
}

void login_enqueue(int fd, sem_t *items_sem) {
	struct login *l = (struct login *)calloc(1, sizeof(struct login));

	l->fd = fd;
	l->next = NULL;

	if(login_head == NULL) {
		login_head = l;
		login_tail = l;
	} else {
		login_tail->next = l;
		login_tail = l;
	}
	
	sem_post(items_sem);
}

int login_dequeue() {
	int fd;
	struct login *l = login_head;

	if(login_head == NULL) {
		return -1;
	}

	fd = login_head->fd;
	login_head = login_head->next;
	free(l);

	return fd;
}