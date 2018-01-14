#include "client.h"

struct chat *head = NULL;

/* ---------- CLIENT-CHAT FUNCTIONS ---------- */

/** This method parses the chat commands so we can easily send the
  * message to the server.
  *
  * @param fd - The file descriptor we read from
  * @param socket_fd - The socket file descriptor for the server
  * @param other_name - The other contact's name
  */
void parse_chat_commands(int fd, int socket_fd, char *other_name) {
	int n, i;
	char buf[MAX_BUF_SIZE], send_to_stdin[MAX_BUF_SIZE];
	memset(buf, 0, MAX_BUF_SIZE);
	memset(send_to_stdin, 0, MAX_BUF_SIZE);

	if ((n = read(fd, buf, MAX_BUF_SIZE)) > 0) {
		for (i = 0; i < MAX_BUF_SIZE; i++) {
			if (buf[i] == '\n') {
				buf[i++] = '\r';
				buf[i++] = '\n';
				buf[i++] = '\r';
				buf[i++] = '\n';
				break;
			}
		}

		strcat(send_to_stdin, CLIENT_CHAT);
		strcat(send_to_stdin, other_name);
		strcat(send_to_stdin, " ");
		strcat(send_to_stdin, client_name);
		strcat(send_to_stdin, " ");
		strcat(send_to_stdin, buf);
		write(socket_fd, send_to_stdin, strlen(send_to_stdin));

	}
}

/** This method, if we don't already have a chat open with the two people,
  * creates the chat box. If  we do have the chat open, it goes to a method
  * which finds the socket for the specified chat, and then prints the message
  * to the screen.
  *
  * @param message - The message for the chat
  * @param other_name - The name of the other person
  * @param audit_fd - The socket file descriptor to the audit log
  * @param sender - The sender of the message (for "<" or ">")
  */
void chat_message_handler(char *message, char *other_name, int audit_fd, bool sender) {
	//see if we already have the chat window open
	struct chat *current = head;
	while (current) {
		if (strcmp(current->other_person, other_name) == 0) {
			print_message_to_chat(message, current->socket_fd, sender);
			chat_audit_handler(audit_fd, message, current->other_person, sender);
			return;
		}
		current = current->next;
	}

	//add a new chat struct
	struct chat *new_chat;
	new_chat = (struct chat *)malloc(sizeof(struct chat));
	memset(new_chat->other_person, 0, 100);
	strcat(new_chat->other_person, other_name);

	//sockets, fork pid, status, string file descriptor
	int sockets[2], pid, status;
	char socket[5], audit[5];

	//create a socket pair
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem creating a socketpair." ANSI_DEFAULT_COLOR "\n");
		return;
	}

	//add the rest and place it in the front
	new_chat->socket_fd = sockets[1];
	new_chat->next = head;
	head = new_chat;

	//fork to make the chat program
	if ((pid = fork()) < 0) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "There was a problem forking." ANSI_DEFAULT_COLOR "\n");
		return;
	}
	else if (pid == 0) {
		//child process; exec xterm
		sprintf(socket, "%d", sockets[0]);
		sprintf(audit, "%d", audit_fd);
		close(sockets[1]);

		char *arguments[] = { "/usr/bin/xterm", "-geometry", "100x40+10", "-T", other_name,
							"-e", "./chat", socket, audit, client_name, NULL };
		execvp(arguments[0], arguments);
	}
	else {
		//parent process; don't wait
		new_chat->pid = pid;
		close(sockets[0]);

		print_message_to_chat(message, sockets[1], sender);
		chat_audit_handler(audit_fd, message, new_chat->other_person, sender);
		add_fd(sockets[1], other_name);

		waitpid(pid, &status, WNOHANG);
	}
}

/** This message, given the chat message, sends the desired output to the chat
  * window of the corresponding chat.
  *
  * @param message - The message to be sent
  * @param socket_fd - The socket file descriptor of the chat window
  * @param sender - true if this person is the sender, false otherwise
  */
void print_message_to_chat(char *message, int socket_fd, bool sender) {
	char output_buf[MAX_BUF_SIZE];
	memset(output_buf, 0, MAX_BUF_SIZE);

	//set the buffer to send
	if (sender)
		strcat(output_buf, "< ");
	else
		strcat(output_buf, "> ");
	strcat(output_buf, message);
	strcat(output_buf, "\n");

	//and send it to the socket
	write(socket_fd, output_buf, strlen(output_buf));
}

/** This method ends the chat terminal with the user, if the user
  * just logs off. If it is able to find the user and if the user
  * has been talking to you, it prints a special message to the 
  * terminal. Upon pressing enter, the terminal exits.
  *
  * @param user - The name of the user
  */
void end_chat_term(char *user) {
	char output_buf[MAX_BUF_SIZE];
	memset(output_buf, 0, MAX_BUF_SIZE);

	struct chat *current = head;
	while (current) {
		if (strcmp(user, current->other_person) == 0) {
			strcat(output_buf, "/End User ");
			strcat(output_buf, user);
			strcat(output_buf, " has logged off. You can no longer communicate.\n");
			strcat(output_buf, "Press enter to continue. . .");
			write(current->socket_fd, output_buf, strlen(output_buf));
			break;
		}
		else
			current = current->next;
	}
}

/** This method prints the chat messages to the audit log.
  *
  * @param audit_fd - The socket file descriptor to the audit log
  * @param message - The message
  * @param other_name - The other person's name
  * @param sender - True if the user is the sender of the message
  */
void chat_audit_handler(int audit_fd, char *message, char *other_name, bool sender) {
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
	strcat(audit_buffer, ", MSG, ");
	if (sender)
		strcat(audit_buffer, "to, ");
	else
		strcat(audit_buffer, "from, ");
	strcat(audit_buffer, other_name);
	strcat(audit_buffer, ", ");
	strcat(audit_buffer, message);
	strcat(audit_buffer, "\n");

	//write it
	flock(audit_fd, LOCK_EX);
	write(audit_fd, audit_buffer, strlen(audit_buffer));
	flock(audit_fd, LOCK_UN);
}