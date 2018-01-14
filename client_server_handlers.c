#include "client.h"

/* ---------- CLIENT-SERVER FUNCTIONS ---------- */

/** This method parses the client-server functions. It is given the
  * buffer to parse and uses strtok to access the correct handler.
  *
  * @param socket_fd - The socket file descriptor to the server
  * @param audit_fd - The socket file descriptor to the audit log
  * @param verbose - Option to print all protocol verbs and content
  * @return int - EXIT_SUCCESS so the client knows to terminate
  */
int parse_server_commands(int socket_fd, int audit_fd, bool verbose) {
	int n, safe_termination;
	char buf[MAX_BUF_SIZE];
	memset(buf, 0, MAX_BUF_SIZE);

	if ((n = read(socket_fd, buf, MAX_BUF_SIZE)) > 0) {
		if (strncmp(buf, SERVER_TIME, strlen(SERVER_TIME)) == 0)
			emit_handler(buf, verbose);
		else if (strncmp(buf, "BYE ", 4) == 0) {
			safe_termination = bye_handler(buf, audit_fd, verbose);
			if (safe_termination == EXIT_SUCCESS)
				return -1;
		}
		else if (strncmp(buf, SERVER_LISTU, strlen(SERVER_LISTU)) == 0)
			users_handler(buf, verbose);
		else if (strncmp(buf, CLIENT_CHAT, strlen(CLIENT_CHAT)) == 0)
			messages_handler(buf, audit_fd, verbose);
		else if (strncmp(buf, CLIENT_SERVER_ERROR, strlen(CLIENT_SERVER_ERROR)) == 0)
			error_handler(buf, audit_fd, verbose);
		else if (strncmp(buf, SERVER_UOFF, strlen(SERVER_UOFF)) == 0) {
			uoff_handler(buf, verbose);
		}
		else {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Server command not available" ANSI_DEFAULT_COLOR "\n\n");
		}
	}

	return EXIT_SUCCESS;
}


/* ---------- CLIENT-SERVER HELPER FUNCTIONS ---------- */

/** This method gets the time from the server and prints it out in a neat
  * organized way.
  *
  * @param buffer - The char buffer reading the server
  * @param verbose - Option to print all protocol verbs and content
  */
void emit_handler(char *buffer, bool verbose) {
	char *token;
	time_t timer;
	int hour = 0, minute = 0, second = 0;

	//get the time
	token = strtok(buffer, " \r\n");
	token = strtok(NULL, " ");

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Incoming: %s%s" ANSI_DEFAULT_COLOR "\n", SERVER_TIME, token);

	if ((timer = atoi(token)) == 0) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem retrieving the time." ANSI_DEFAULT_COLOR "\n");
		return;
	}

	//and make sure the rest is the end symbols (sans the space at the beginning)
	char ptr[] = "\r\n\r\n";
	token = strtok(NULL, " ");
	if (strcmp(ptr, token) != 0) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem receiving the end symbols." ANSI_DEFAULT_COLOR "\n");
		return;
	}

	//conversion; hours first
	while (1) {
		if ((timer - 3600) >= 0) {
			hour++;
			timer -= 3600;
		}
		else
			break;
	}

	//minutes next
	while (1) {
		if ((timer - 60) >= 0) {
			minute++;
			timer -= 60;
		}
		else
			break;
	}

	//leftovers are seconds
	second = timer;

	//now print!
	sfwrite(&lock, stdout, "connected for %d hour(s), %d minute(s), and %d second(s)\n",
		hour, minute, second);
}

/** This method checks to see if we should close the client (anytime we
  * receive "BYE \r\n\r\n").
  *
  * @param buffer - The char buffer from the server
  * @param audit_fd - The socket file descriptor to the audit log
  * @param verbose - Option to print all protocol verbs and content
  * @return int - EXIT_SUCCESS so we can terminate, EXIT_FAILURE so we can't
  */
int bye_handler(char *buffer, int audit_fd, bool verbose) {
	char input_buf[MAX_BUF_SIZE];
	memset(input_buf, 0, MAX_BUF_SIZE);

	strcat(input_buf, CLIENT_SERVER_BYE);
	strcat(input_buf, END_SYMBOLS);

	if (strcmp(buffer, input_buf) != 0) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem receiving BYE." ANSI_DEFAULT_COLOR "\n");
		return EXIT_FAILURE;
	}

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Incoming: %s" ANSI_DEFAULT_COLOR "\n", CLIENT_SERVER_BYE);

	server_audit_handler(audit_fd, "logout");

	//now, we return EXIT_SUCCESS so that the client knows it can terminate
	return EXIT_SUCCESS;
}

/** This method gets all the users currently logged in and running from
  * the server, and prints them out.
  *
  * @param buffer - The char buffer from the server
  * @param verbose - Option to print all protocol verbs and content
  */
void users_handler(char *buffer, bool verbose) {
	char user_buf[MAX_BUF_SIZE];
	char *token;
	memset(user_buf, 0, MAX_BUF_SIZE);

	if (verbose) {
		int i = 6, j = 0;
		while (1) {
			//fill the user buffer with all the users, excluding "\r\n "
			if ((buffer[i] != '\r' && buffer[i] != '\n') ||
				(buffer[i] == ' ' && buffer[i - 1] != '\n'))
				user_buf[j++] = buffer[i];
			else if (buffer[i] == '\r' && buffer[i - 2] == '\r')
				break;
			i++;
		}

		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Incoming: %s%s" ANSI_DEFAULT_COLOR "\n", SERVER_LISTU, user_buf);
	}

	//now, get ALL users and print them out; first line gets "UTSIL user1", second gets "user1"
	token = strtok(buffer, " \r\n");
	token = strtok(NULL, " ");

	sfwrite(&lock, stdout, "Current users online: \n");

	//get all the users now, and print them
	while (token) {
		sfwrite(&lock, stdout, "%s\n", token);
		token = strtok(NULL, " \r\n");
	}
}

/** This method deals with making a chat program. It calls xterm, which
  * will open up a new chat, and call the chat program. The format that
  * the buffer is originally in is "MSG <to> <from> <message>".
  *
  * @param buffer - The char buffer from the server
  * @param audit_fd - The socket file descriptor to the audit log
  * @param verbose - Option to print all protocol verbs and content
  */
void messages_handler(char *buffer, int audit_fd, bool verbose) {
	char original_buffer[MAX_BUF_SIZE];
	char *token, *other_party_name;
	bool sender = false;
	memset(original_buffer, 0, MAX_BUF_SIZE);
	strcat(original_buffer, buffer);

	token = strtok(buffer, "\r\n");

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Incoming: %s" ANSI_DEFAULT_COLOR "\n", token);

	//get the other client's name; gets "MSG", and then the name
	token = strtok(original_buffer, " ");
	if ((token = strtok(NULL, " ")) == NULL) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error: insufficient arguments for /chat" ANSI_DEFAULT_COLOR "\n");
		return;
	}

	if (strcmp(token, client_name) == 0) {
		if ((token = strtok(NULL, " ")) == NULL) {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error: insufficient arguments for /chat" ANSI_DEFAULT_COLOR "\n");
			return;
		}
		
		other_party_name = token;
	}
	else {
		other_party_name = token;
		sender = true;
		if ((token = strtok(NULL, " ")) == NULL) {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error: insufficient arguments for /chat" ANSI_DEFAULT_COLOR "\n");
			return;
		}
	}

	//and get the message
	if ((token = strtok(NULL, "\r\n")) == NULL) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error: insufficient arguments for /chat" ANSI_DEFAULT_COLOR "\n");
		return;
	}

	//now, go to a separate chat method
	chat_message_handler(token, other_party_name, audit_fd, sender);
}

/** This method handles the error messages from the server.
  *
  * @param buffer - The buffer from the server
  * @param audit_fd - The socket file descriptor to the audit log
  * @param verbose - Option to print all the protocols and contents
  */
void error_handler(char *buffer, int audit_fd, bool verbose) {
	buffer = strtok(buffer, "\r\n");

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Incoming: %s" ANSI_DEFAULT_COLOR "\n", buffer);

	//handle correctly
	if (strncmp(buffer, ERROR_NOT_AVAIL, strlen(ERROR_NOT_AVAIL)) == 0) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error: user is not available." ANSI_DEFAULT_COLOR "\n");
		server_audit_handler(audit_fd, ERROR_NOT_AVAIL);
	}
	else {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "An unknown error occurred." ANSI_DEFAULT_COLOR "\n");
		server_audit_handler(audit_fd, ERROR_NOT_AVAIL);
	}
}

/** This method handles the UOFF verb. It sends the user to another method,
  * which checks to see if there is a chat window open for this user.
  *
  * @param buffer - The buffer from the server
  * @param verbose - Option to print all the protocols and contents
  */
void uoff_handler(char *buffer, bool verbose) {
	buffer = strtok(buffer, "\r\n");

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Incoming: %s" ANSI_DEFAULT_COLOR "\n", buffer);

	char *user = strtok(buffer, " ");
	user = strtok(NULL, " ");
	end_chat_term(user);
}

/** This method prints the audit message for the server command:
  * on logout and on error.
  *
  * @param audit_fd - The socket file descriptor to the audit log
  * @param command - The command -- either "logout" or error message
  */
void server_audit_handler(int audit_fd, char *command) {
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

	if (strcmp(command, "logout") == 0) {
		strcat(audit_buffer, ", LOGOUT, ");
		if (intentional)
			strcat(audit_buffer, "intentional\n");
		else
			strcat(audit_buffer, ", error\n");
	}
	else {
		strcat(audit_buffer, ", ERR, ");
		strcat(audit_buffer, command);
		strcat(audit_buffer, "\n");
	}

	//write it
	flock(audit_fd, LOCK_EX);
	write(audit_fd, audit_buffer, strlen(audit_buffer));
	flock(audit_fd, LOCK_UN);
}