#include "client.h"

void audit_close_handler(int, char *);

int main(int argc, char *argv[]) {
	int socket_fd = atoi(argv[1]);
	int audit_fd = atoi(argv[2]);
	char *client_name = argv[3];
	char read_buf[MAX_BUF_SIZE];
	fd_set read_set, ready_set;

	FD_ZERO(&read_set);
	FD_SET(STDIN_FILENO, &read_set);
	FD_SET(socket_fd, &read_set);

	while (1) {
		ready_set = read_set;
		memset(read_buf, 0, MAX_BUF_SIZE);

		pselect(socket_fd + 1, &ready_set, NULL, NULL, NULL, NULL);

		if (FD_ISSET(STDIN_FILENO, &ready_set)) {
			read(STDIN_FILENO, read_buf, MAX_BUF_SIZE);
			if (strncmp(read_buf, "/close", 6) == 0) {
				audit_close_handler(audit_fd, client_name);
				close(socket_fd);
				exit(EXIT_SUCCESS);
			}
			write(socket_fd, read_buf, strlen(read_buf));
		}
		if (FD_ISSET(socket_fd, &ready_set)) {
			read(socket_fd, read_buf, MAX_BUF_SIZE);
			int done = 0;

			if (strncmp(read_buf, "/End ", 5) == 0) {
				done = 1;
				
				int i = 0;
				while (read_buf[i + 5] != '\0') {
					read_buf[i] = read_buf[i + 5];
					i++;
				}
				read_buf[i++] = '\0';
				read_buf[i++] = '\0';
				read_buf[i++] = '\0';
				read_buf[i++] = '\0';
				read_buf[i++] = '\0';
			}
			
			write(STDOUT_FILENO, read_buf, strlen(read_buf));

			if (done) {
				while (getchar() != '\n');
				exit(EXIT_SUCCESS);
			}
		}
	}

	return EXIT_SUCCESS;
}

/** This method sends the close command to the audit log.
  *
  * @param audit_fd - The socket file descriptor of the audit log
  * @param client_name - The name of the user
  */
void audit_close_handler(int audit_fd, char *client_name) {
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
	strcat(audit_buffer, ", CMD, /close, success, chat\n");

	//write it
	flock(audit_fd, LOCK_EX);
	write(audit_fd, audit_buffer, strlen(audit_buffer));
	flock(audit_fd, LOCK_UN);
}