#include "client.h"

/* ---------- CLIENT STDIN FUNCTIONS ---------- */

/** This method parses the stdin functions. It is given the buffer to
  * parse and uses strtok to access the correct handler.
  *
  * @param socket_fd - The socket file descriptor to the server
  * @param audit_fd - The socket file descriptor to the audit log
  * @param verbose - Option to print all protocol verbs and content
  */
void parse_stdin_commands(int socket_fd, int audit_fd, bool verbose) {
	int n, i, sof;
	char buf[MAX_BUF_SIZE];
	memset(buf, 0, MAX_BUF_SIZE);

	if ((n = read(STDIN_FILENO, buf, MAX_BUF_SIZE)) > 0) {
		for (i = 0; i < MAX_BUF_SIZE; i++) {
			if (buf[i] == '\n') {
				buf[i] = '\0';
				break;
			}
		}
		
		if (strncmp(buf, "/time", 5) == 0) {
			time_handler(socket_fd, verbose);
			stdin_audit_handler(audit_fd, buf, true);
		}
		else if (strncmp(buf, "/help", 5) == 0) {
			help_handler();
			stdin_audit_handler(audit_fd, buf, true);
		}
		else if (strncmp(buf, "/logout", 7) == 0) {
			logout_handler(socket_fd, verbose);
			stdin_audit_handler(audit_fd, buf, true);
		}
		else if (strncmp(buf, "/listu", 6) == 0) {
			listu_handler(socket_fd, verbose);
			stdin_audit_handler(audit_fd, buf, true);
		}
		else if (strncmp(buf, "/chat ", 6) == 0) {
			sof = chat_handler(buf, socket_fd, verbose);
			stdin_audit_handler(audit_fd, buf, sof);
		}
		else if (strncmp(buf, "/audit", 6) == 0) {
			audit_handler();
			stdin_audit_handler(audit_fd, buf, true);
		}
		else {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Command %s not available" ANSI_DEFAULT_COLOR "\n\n", buf);
			if (buf[0] != '\0')
				stdin_audit_handler(audit_fd, buf, false);
			help_handler();
		}
	}
}


/* ---------- CLIENT STDIN HELPER FUNCTIONS ---------- */

/** This method gets the amount of time the client has been connected.
  * The server will return the time in seconds, and the client converts
  * it to readable time.
  *
  * @param socket_fd - The socket file descriptor to the server
  * @param verbose - Option to print all protocol verbs and content
  */
void time_handler(int socket_fd, bool verbose) {
	char output_buf[MAX_BUF_SIZE];
	memset(output_buf, 0, MAX_BUF_SIZE);

	//send "TIME \r\n\r\n" to server; confirm we get correct data back
	strcat(output_buf, CLIENT_TIME);
	strcat(output_buf, END_SYMBOLS);

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Outgoing: %s" ANSI_DEFAULT_COLOR "\n", CLIENT_TIME);

	if (write(socket_fd, output_buf, strlen(output_buf)) != strlen(output_buf)) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem writing TIME." ANSI_DEFAULT_COLOR "\n");
		return;
	}
}

/** This method prints out all the possible commands that the
  * client accepts.
  */
void help_handler(void) {
	sfwrite(&lock, stdout, "Available commands for client:\n\n");
	sfwrite(&lock, stdout, "    /chat <to> <msg>       Sends a message to another user, if user is logged in.\n");
	sfwrite(&lock, stdout, "    /help                  Lists all the commands that the client accepts.\n");
	sfwrite(&lock, stdout, "    /listu                 Lists all the users currently connected.\n");
	sfwrite(&lock, stdout, "    /logout                Disconnects the client from the server.\n");
	sfwrite(&lock, stdout, "    /time                  Prints how long the client has been connected.\n\n");
}

/** This method attempts to log the client out of the server.
  *
  * @param socket_fd - The socket file descriptor to the server
  * @param verbose - Option to print all protocol verbs and content
  */
void logout_handler(int socket_fd, bool verbose) {
	char output_buf[MAX_BUF_SIZE];
	memset(output_buf, 0, MAX_BUF_SIZE);

	//send "BYE \r\n\r\n" to the server; wait for confirmation
	strcat(output_buf, CLIENT_SERVER_BYE);
	strcat(output_buf, END_SYMBOLS);
	intentional = true;

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Outgoing: %s" ANSI_DEFAULT_COLOR "\n", CLIENT_SERVER_BYE);

	if (write(socket_fd, output_buf, strlen(output_buf)) != strlen(output_buf)) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem writing BYE." ANSI_DEFAULT_COLOR "\n");
		return;
	}
}

/** This method receives and prints all the currently connected
  * users on the server. The users will be printed one per line.
  *
  * @param socket_fd - The socket file descriptor to the server
  * @param verbose - Option to print all protocol verbs and content
  */
void listu_handler(int socket_fd, bool verbose) {
	char output_buf[MAX_BUF_SIZE];
	memset(output_buf, 0, MAX_BUF_SIZE);

	//send "LISTU \r\n\r\n" to the server; wait for receipt
	strcat(output_buf, CLIENT_LISTU);
	strcat(output_buf, END_SYMBOLS);

	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Outgoing: %s" ANSI_DEFAULT_COLOR "\n", CLIENT_LISTU);

	if (write(socket_fd, output_buf, strlen(output_buf)) != strlen(output_buf)) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem writing LISTU." ANSI_DEFAULT_COLOR "\n");
		return;
	}
}

/** This method attempts to add a chat program so a user can talk to
  * another currently online user. The buffer gets sent as "/chat TO msg".
  * We send to the server "MSG <TO> <FROM> <MESSAGE>". From is the global
  * variable "client_name."
  *
  * @param buffer - The buffer sent from the user (stdin)
  * @param socket_fd - The socket file descriptor to the server
  * @param verbose - Option to print all protocol verbs and content
  * @return int - 0 on failure, 1 on success
  */
int chat_handler(char *buffer, int socket_fd, bool verbose) {
	char *token, output_buf[MAX_BUF_SIZE];
	memset(output_buf, 0, MAX_BUF_SIZE);

	//place MSG into the buffer
	strcat(output_buf, CLIENT_CHAT);

	//get the receiver; first strtok gets "/chat", second gets other user
	token = strtok(buffer, " \r\n\r\n");
	token = strtok(NULL, " ");

	//check for validity
	if (token == NULL) {
		sfwrite(&lock, stdout, ANSI_ERRORS_COLOR "There is an error in parsing the name parameter." ANSI_DEFAULT_COLOR "\n");
		return 0;
	}

	strcat(output_buf, token);

	//add " <FROM> "
	strcat(output_buf, " ");
	strcat(output_buf, client_name);
	strcat(output_buf, " ");

	//next is the message
	token = strtok(NULL, "\r\n");

	//check for validity
	if (token == NULL) {
		sfwrite(&lock, stdout, ANSI_ERRORS_COLOR "There is an error in parsing the message parameter." ANSI_DEFAULT_COLOR "\n");
		return 0;
	}

	strcat(output_buf, token);

	//handle verbose
	if (verbose)
		sfwrite(&lock, stdout, ANSI_VERBOSE_COLOR "Outgoing: %s" ANSI_DEFAULT_COLOR "\n", output_buf);

	//and then the end symbols
	strcat(output_buf, END_SYMBOLS);

	if (write(socket_fd, output_buf, strlen(output_buf)) != strlen(output_buf)) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem writing MSG." ANSI_DEFAULT_COLOR "\n");
		return 0;
	}
	return 1;
}

/** This method gets the audit log and prints out its contents, from
  * oldest to newest. It locks the data so that others can still use
  * it, but it only gets complete logs.
  */
void audit_handler(void) {
	FILE *audit;
	int audit_read_fd;
	size_t length = MAX_INPUT;
	char *line = (char *)malloc(MAX_INPUT);

	//open for reading
	if ((audit = fopen(audit_file, "r")) == NULL) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error opening audit log" ANSI_DEFAULT_COLOR "\n");
		return;
	}

	//lock it
	audit_read_fd = fileno(audit);
	flock(audit_read_fd, LOCK_SH);

	//read it line-by-line and print the contents
	while (getline(&line, &length, audit) != -1) {
		sfwrite(&lock, stdout, line, strlen(line));
	}

	//unlock and close/free everything
	flock(audit_read_fd, LOCK_UN);
	fclose(audit);
	close(audit_read_fd);
	free(line);
}

/** This method handles printing the stdin commands from the client
  * to the audit log.
  *
  * @param audit_fd - The socket file descriptor to the audit log
  * @param cmd - The command entered
  * @param success - True if the command was successful
  */
void stdin_audit_handler(int audit_fd, char *cmd, bool success) {
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
	strcat(audit_buffer, ", CMD, ");
	strcat(audit_buffer, cmd);
	if (success)
		strcat(audit_buffer, ", success, ");
	else
		strcat(audit_buffer, ", failure, ");
	strcat(audit_buffer, "client\n");

	//write it
	flock(audit_fd, LOCK_EX);
	write(audit_fd, audit_buffer, strlen(audit_buffer));
	flock(audit_fd, LOCK_UN);
}