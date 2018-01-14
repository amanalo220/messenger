#include "client.h"

fd_set read_set;
int listen_fd;
struct fd *fd_head = NULL;

/* ---------- CLIENT LOGIN FUNCTION ---------- */

/** This method takes initiates the login feature for the client.
  * It tries to log in to the server, and returns success or failure.
  * On failure, it returns the type of failure, so the client can
  * handle it accordingly.
  *
  * @param socket_fd - The socket file descriptor to the server
  * @param audit_fd - The socket file descriptor to the audit log
  * @param name - The client's username
  * @param verbose - Option to print all protocol verbs and content
  * @param new_user - Option to say I'm a new user
  * @return int - EXIT_SUCCESS, EXIT_FAILURE, -1 (wolfie connection), -2 (username connection), -3 (password),
  *				 -4 (not avail), -5 (unknown error)
  */
int login_protocol(int socket_fd, int audit_fd, char *name, bool verbose, bool new_user) {
	char read_buf[MAX_BUF_SIZE], input_buf[MAX_BUF_SIZE], output_buf[MAX_BUF_SIZE];
	char *token, *password, *ptr;
	memset(read_buf, 0, MAX_BUF_SIZE);
	memset(input_buf, 0, MAX_BUF_SIZE);
	memset(output_buf, 0, MAX_BUF_SIZE);

	//send "WOLFIE \r\n\r\n" to server; confirm we get correct output back
	strcat(input_buf, CLIENT_WOLFIE);
	strcat(input_buf, END_SYMBOLS);

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Outgoing: %s" ANSI_DEFAULT_COLOR "\n", CLIENT_WOLFIE);
	
	if (write(socket_fd, input_buf, strlen(input_buf)) != strlen(input_buf)) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem writing WOLFIE." ANSI_DEFAULT_COLOR "\n");
		return EXIT_FAILURE;
	}
	read(socket_fd, read_buf, MAX_BUF_SIZE);

	strcat(output_buf, SERVER_WOLFIE);
	strcat(output_buf, END_SYMBOLS);
	if (strcmp(read_buf, output_buf) != 0) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem connecting: WOLFIE" ANSI_DEFAULT_COLOR "\n");
		return -1;
	}

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Incoming: %s" ANSI_DEFAULT_COLOR "\n", SERVER_WOLFIE);

	if (new_user) {
		return login_new_user_protocol(socket_fd, audit_fd, name, verbose);
	}

	memset(read_buf, 0, MAX_BUF_SIZE);
	memset(input_buf, 0, MAX_BUF_SIZE);
	memset(output_buf, 0, MAX_BUF_SIZE);
	
	//send "IAM <name> \r\n\r\n" to server; confirm we get correct output back
	strcat(input_buf, CLIENT_IAM);
	strcat(input_buf, name);
	strcat(input_buf, END_SYMBOLS);

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Outgoing: %s%s" ANSI_DEFAULT_COLOR "\n", CLIENT_IAM, name);

	if (write(socket_fd, input_buf, strlen(input_buf)) != strlen(input_buf)) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem writing IAM." ANSI_DEFAULT_COLOR "\n");
		return EXIT_FAILURE;
	}

	//make sure we get AUTH back
	strcat(output_buf, SERVER_AUTH);
	strcat(output_buf, name);
	strcat(output_buf, END_SYMBOLS);
	read(socket_fd, read_buf, MAX_BUF_SIZE);

	if (strcmp(read_buf, output_buf) != 0) {
		if (strncmp(read_buf, ERROR_USERNAME, strlen(ERROR_USERNAME)) == 0) {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem connecting: %s" ANSI_DEFAULT_COLOR "\n", client_name);
			return -2;
		}
		else if (strncmp(read_buf, ERROR_NOT_AVAIL, strlen(ERROR_NOT_AVAIL)) == 0) {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem connecting: %s" ANSI_DEFAULT_COLOR "\n", client_name);
			return -4;
		}
		else
			return -5;
	}

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Incoming: %s%s" ANSI_DEFAULT_COLOR "\n", SERVER_AUTH, name);

	memset(read_buf, 0, MAX_BUF_SIZE);
	memset(input_buf, 0, MAX_BUF_SIZE);
	memset(output_buf, 0, MAX_BUF_SIZE);

	//enter password
	password = getpass("Please enter your password: ");
	strcat(input_buf, CLIENT_PASS);
	strcat(input_buf, password);
	strcat(input_buf, END_SYMBOLS);

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Outgoing: %s[password]" ANSI_DEFAULT_COLOR "\n", CLIENT_PASS);

	if (write(socket_fd, input_buf, strlen(input_buf)) != strlen(input_buf)) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem writing IAM." ANSI_DEFAULT_COLOR "\n");
		return EXIT_FAILURE;
	}

	memset(input_buf, 0, MAX_BUF_SIZE);

	//receive SSAP HI and MOTD
	read(socket_fd, read_buf, MAX_BUF_SIZE);
	//printf("%s\n", read_buf);
	strcat(input_buf, SERVER_PASS);
	strcat(input_buf, " ");
	
	ptr = read_buf;
	
	token = strtok_r(read_buf, "\r\n", &ptr);

	//check for SSAP
	if(strcmp(token, input_buf) != 0) {
		if (strncmp(read_buf, ERROR_PASSWORD, strlen(ERROR_PASSWORD)) == 0) {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem connecting: PASSWORD" ANSI_DEFAULT_COLOR "\n");
			return -3;
		}
		else
			return -5;
	}

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Incoming: %s" ANSI_DEFAULT_COLOR "\n", SERVER_PASS);

	//now, get "HI"
	ptr += 3;

	memset(input_buf, 0, MAX_BUF_SIZE);
	memset(output_buf, 0, MAX_BUF_SIZE);

	if (ptr == NULL || *ptr == 0) {
		memset(read_buf, 0, MAX_BUF_SIZE);
		read(socket_fd, read_buf, MAX_BUF_SIZE);
		token = strtok_r(read_buf, "\r\n", &ptr);
		strcat(output_buf, token);
		ptr += 3;
	}
	else {
		int i = 0;
		while (*ptr != 13 && *ptr != 0) {
			output_buf[i++] = *ptr;
			ptr++;
		}

		ptr += 4;
	}

	strcat(input_buf, SERVER_HI);
	strcat(input_buf, name);
	strcat(input_buf, " ");

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Incoming: %s" ANSI_DEFAULT_COLOR "\n", input_buf);
	
	//check for HI
	if (strcmp(output_buf, input_buf) != 0) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem connecting: HI." ANSI_DEFAULT_COLOR "\n");
		return -5;
	}

	memset(input_buf, 0, MAX_BUF_SIZE);
	memset(output_buf, 0, MAX_BUF_SIZE);

	if (ptr == NULL || *ptr == 0) {
		memset(read_buf, 0, MAX_BUF_SIZE);
		read(socket_fd, read_buf, MAX_BUF_SIZE);
		token = strtok_r(read_buf, "\r\n", &ptr);
		strcat(output_buf, token);
		ptr += 3;
	}
	else {
		int i = 0;
		while (*ptr != 10 && *ptr != 0) {
			output_buf[i++] = *ptr;
			ptr++;
		}
	}

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Incoming: %s" ANSI_DEFAULT_COLOR "\n", output_buf);

	token = strtok_r(output_buf, " ", &ptr);
	token = strtok_r(NULL, " ", &ptr);

	//print the message
	sfwrite(&lock, stdout, "%s\n", token);

	login_audit_handler(audit_fd, token);

	return EXIT_SUCCESS;
}

/** This method attempts to create a new user, using "-c" as an argument. If the password
  * is bad or anything goes wrong, the client is denied all rights and
  * exits as cleanly as it can.
  *
  * @param socket_fd - The socket file descriptor to the server
  * @param audit_fd - The socket file descriptor to the audit log
  * @param name - The client's username
  * @param verbose - Option to print all protocol verbs and content
  * @param new_user - Option to say I'm a new user
  * @return int - EXIT_SUCCESS, EXIT_FAILURE, -1 (wolfie connection), -2 (username connection), -3 (password),
  *				 -4 (not avail), -5 (unknown error)
  */
int login_new_user_protocol(int socket_fd, int audit_fd, char *name, bool verbose) {
	char read_buf[MAX_BUF_SIZE], input_buf[MAX_BUF_SIZE], output_buf[MAX_BUF_SIZE];
	char *token, *password, *ptr;
	memset(read_buf, 0, MAX_BUF_SIZE);
	memset(input_buf, 0, MAX_BUF_SIZE);
	memset(output_buf, 0, MAX_BUF_SIZE);

	//send IAMNEW to the server
	strcat(input_buf, CLIENT_IAMNEW);
	strcat(input_buf, name);
	strcat(input_buf, END_SYMBOLS);

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Outgoing: %s%s" ANSI_DEFAULT_COLOR "\n", CLIENT_IAMNEW, name);

	if (write(socket_fd, input_buf, strlen(input_buf)) != strlen(input_buf)) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem writing IAMNEW." ANSI_DEFAULT_COLOR "\n");
		return EXIT_FAILURE;
	}

	//receive HINEW if it worked
	strcat(output_buf, SERVER_HINEW);
	strcat(output_buf, name);
	strcat(output_buf, END_SYMBOLS);
	read(socket_fd, read_buf, MAX_BUF_SIZE);
	
	if (strcmp(read_buf, output_buf) != 0) {
		if (strncmp(read_buf, ERROR_USERNAME, strlen(ERROR_USERNAME)) == 0) {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem connecting: %s" ANSI_DEFAULT_COLOR "\n", client_name);
			return -2;
		}
		else
			return -5;
	}

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Incoming: %s%s" ANSI_DEFAULT_COLOR "\n", SERVER_HINEW, name);

	memset(read_buf, 0, MAX_BUF_SIZE);
	memset(input_buf, 0, MAX_BUF_SIZE);
	memset(output_buf, 0, MAX_BUF_SIZE);

	//if we get here, we can set a new password
	password = getpass("Please enter a password: ");
	strcat(output_buf, CLIENT_NEWPASS);
	strcat(output_buf, password);
	strcat(output_buf, END_SYMBOLS);

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Outgoing: %s[password]" ANSI_DEFAULT_COLOR "\n", CLIENT_NEWPASS);

	if (write(socket_fd, output_buf, strlen(output_buf)) != strlen(output_buf)) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem writing PASSWORD." ANSI_DEFAULT_COLOR "\n");
		return EXIT_FAILURE;
	}

	//receive SSAPWEN HI and MOTD
	read(socket_fd, read_buf, MAX_BUF_SIZE);
	//printf("%s\n", read_buf);
	strcat(input_buf, SERVER_NEWPASS);
	strcat(input_buf, " ");
	
	ptr = read_buf;
	
	token = strtok_r(read_buf, "\r\n", &ptr);

	//check for SSAPWEN
	if(strcmp(token, input_buf) != 0) {
		if (strncmp(read_buf, ERROR_PASSWORD, strlen(ERROR_PASSWORD)) == 0) {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem connecting: PASSWORD" ANSI_DEFAULT_COLOR "\n");
			return -3;
		}
		else
			return -5;
	}

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Incoming: %s" ANSI_DEFAULT_COLOR "\n", SERVER_NEWPASS);

	//now, get "HI"
	ptr += 3;

	memset(input_buf, 0, MAX_BUF_SIZE);
	memset(output_buf, 0, MAX_BUF_SIZE);

	if (ptr == NULL || *ptr == 0) {
		memset(read_buf, 0, MAX_BUF_SIZE);
		read(socket_fd, read_buf, MAX_BUF_SIZE);
		token = strtok_r(read_buf, "\r\n", &ptr);
		strcat(output_buf, token);
		ptr += 3;
	}
	else {
		int i = 0;
		while (*ptr != 13 && *ptr != 0) {
			output_buf[i++] = *ptr;
			ptr++;
		}

		ptr += 4;
	}

	strcat(input_buf, SERVER_HI);
	strcat(input_buf, name);
	strcat(input_buf, " ");

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Incoming: %s" ANSI_DEFAULT_COLOR "\n", input_buf);
	
	//check for HI
	if (strcmp(output_buf, input_buf) != 0) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem connecting: HI." ANSI_DEFAULT_COLOR "\n");
		return -5;
	}

	//and finally, get the MOTD
	memset(input_buf, 0, MAX_BUF_SIZE);
	memset(output_buf, 0, MAX_BUF_SIZE);

	if (ptr == NULL || *ptr == 0) {
		memset(read_buf, 0, MAX_BUF_SIZE);
		read(socket_fd, read_buf, MAX_BUF_SIZE);
		token = strtok_r(read_buf, "\r\n", &ptr);
		strcat(output_buf, token);
		ptr += 3;
	}
	else {
		int i = 0;
		while (*ptr != 10 && *ptr != 0) {
			output_buf[i++] = *ptr;
			ptr++;
		}
	}

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Incoming: %s" ANSI_DEFAULT_COLOR "\n", output_buf);

	token = strtok_r(output_buf, " ", &ptr);
	token = strtok_r(NULL, " ", &ptr);

	//print the message
	sfwrite(&lock, stdout, "%s\n", token);

	login_audit_handler(audit_fd, token);

	return EXIT_SUCCESS;
}

/** This method prints the success login audit log to the file
  * using the audit_fd.
  *
  * @param audit_fd - The socket file descriptor to the audit log
  * @param motd - The message of the day
  */
void login_audit_handler(int audit_fd, char *motd) {
	char date_timestamp[50], audit_buffer[MAX_BUF_SIZE];
	time_t current_time;
	struct tm *local_time;
	memset(date_timestamp, 0, 50);
	memset(audit_buffer, 0, MAX_BUF_SIZE);

	//get the time-date stamp
	current_time = time(NULL);
	local_time = localtime(&current_time);
	strftime(date_timestamp, 50, "%x-%I:%M%p", local_time);
			
	//get ready to write it to the file
	strcat(audit_buffer, date_timestamp);
	strcat(audit_buffer, ", ");
	strcat(audit_buffer, client_name);
	strcat(audit_buffer, ", LOGIN, ");
	strcat(audit_buffer, ip);
	strcat(audit_buffer, ":");
	strcat(audit_buffer, port);
	strcat(audit_buffer, ", success, ");
	strcat(audit_buffer, motd);
	strcat(audit_buffer, "\n");

	//write it
	flock(audit_fd, LOCK_EX);
	write(audit_fd, audit_buffer, strlen(audit_buffer));
	flock(audit_fd, LOCK_UN);
}


/* ---------- CLIENT MULTIPLEXING FUNCTION ---------- */

/** This method multiplexes on server and stdin. When it receives 
  * input, it checks to see whether it is from stdin or the server
  * and sends the data to the correct handler.
  *
  * @param socket_fd - The socket file descriptor to the server
  * @param audit_fd - The socket file descriptor to the audit log
  * @param verbose - Option to print all protocol verbs and content
  */
void multiplex(int socket_fd, int audit_fd, bool verbose) {
	listen_fd = socket_fd + 1;
	int safe_termination;
	fd_set ready_set;

	//clear the read set, and add stdin and the socket to read set
	FD_ZERO(&read_set);
	FD_SET(STDIN_FILENO, &read_set);
	FD_SET(socket_fd, &read_set);

	//now, loop!
	while (1) {
		ready_set = read_set;
		pselect(listen_fd, &ready_set, NULL, NULL, NULL, NULL);
		if (FD_ISSET(STDIN_FILENO, &ready_set)) {
			//input from stdin; go to its handler
			parse_stdin_commands(socket_fd, audit_fd, verbose);
		}
		else if (FD_ISSET(socket_fd, &ready_set)) {
			//input from server; go to its handler
			safe_termination = parse_server_commands(socket_fd, audit_fd, verbose);
			if (safe_termination == -1)
				break;
		}
		else {
			//input from chat; go to its handler
			struct chat *current = head;
			while (current) {
				if (FD_ISSET(current->socket_fd, &ready_set)) {
					parse_chat_commands(current->socket_fd, socket_fd, current->other_person);
				}
				current = current->next;
			}
		}
	}
}

/** This method adds a file descriptor to the list of its struct.
  *
  * @param fd_to_add - The fd to add
  * @param other_name - The other person's name
  */
void add_fd(int fd_to_add, char *other_name) {
	FD_SET(fd_to_add, &read_set);

	//update the listen fd for multiplexing
	listen_fd = fd_to_add + 1;
}

/** This method removes a file descriptor from the list of its struct.
  * It also removes the struct from the list.
  *
  * @param pid - The pid of the child
  */
void remove_fd(int pid) {
	struct chat *current = head;
	if (current->pid == pid) {
		//add the struct fd
		struct fd *add = (struct fd *)malloc(sizeof(struct fd));
		add->fd = current->socket_fd;
		add->next = fd_head;
		fd_head = add;

		current = current->next;
		free(head);
		head = current;
	}
	else {
		while (current->next) {
			if (current->next->pid == pid) {
				//add the struct fd
				struct fd *add = (struct fd *)malloc(sizeof(struct fd));
				add->fd = current->socket_fd;
				add->next = fd_head;
				fd_head = add;

				current->next = current->next->next;
				free(current->next);
			}
			else
				current = current->next;
		}
	}
}