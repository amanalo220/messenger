#include "client.h"


void sighandler(int signal) {
	int pid = wait(NULL);
	remove_fd(pid);
}

int main(int argc, char *argv[]) {
	//chat sighandler (when a chat terminal dies)
	signal(SIGCHLD, sighandler);

	int addrinfo, socket_fd, audit_fd;
	struct addrinfo *client, *server, *i;
	bool verbose = false, new_user = false, audit = false;
	intentional = false;
	FILE *output_file;

	//initialize the lock
	if (pthread_mutex_init(&lock, NULL) != 0) {
		fprintf(stderr, ANSI_ERRORS_COLOR "Error: pthread init" ANSI_DEFAULT_COLOR "\n");
		exit(EXIT_FAILURE);
	}

	//check the command-line arguments
	if (argc == 2 || argc == 3) {
		if (strcmp(argv[1], "-h") == 0 || strcmp(argv[2], "-h") == 0) {
			usage_statement();
			pthread_mutex_destroy(&lock);
			exit(EXIT_SUCCESS);
		}
		else {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error: client command line args." ANSI_DEFAULT_COLOR "\n");
			usage_statement();
			pthread_mutex_destroy(&lock);
			exit(EXIT_FAILURE);
		}
	}
	else if (argc < 4) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error: client command line args." ANSI_DEFAULT_COLOR "\n");
		usage_statement();
		pthread_mutex_destroy(&lock);
		exit(EXIT_FAILURE);
	}

	//handle the help menu, if there is one.
	int args;
	for (args = 1; args < argc; args++) {
		if (strcmp(argv[args], "-h") == 0) {
			usage_statement();
			pthread_mutex_destroy(&lock);
			exit(EXIT_SUCCESS);
		}
		else if (strcmp(argv[args], "-v") == 0)
			verbose = true;
		else if (strcmp(argv[args], "-c") == 0)
			new_user = true;
		else if (strcmp(argv[args], "-a") == 0) {
			output_file = fopen(argv[args + 1], "a");
			audit_file = argv[args + 1];
			audit_fd = fileno(output_file);
			audit = true;
		}
	}

	if (!audit) {
		output_file = fopen("audit.log", "a");
		audit_file = "audit.log";
		audit_fd = fileno(output_file);
	}

	//set the data
	client_name = argv[argc - 3];
	ip = argv[argc - 2];
	port = argv[argc - 1];

	//clear data; get the address info, allocates a linked list of addrinfo structs
	memset(&client, 0, sizeof(client));
	if ((addrinfo = getaddrinfo(ip, port, client, &server)) != 0) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error: gettaddrinfo: %s" ANSI_DEFAULT_COLOR  "\n", gai_strerror(addrinfo));
		pthread_mutex_destroy(&lock);
		exit(EXIT_FAILURE);
	}

	//now, loop and get the socket
	for (i = server; i != NULL; i = i->ai_next) {
		if ((socket_fd = socket(i->ai_family, i->ai_socktype, i->ai_protocol)) < 0) {
			int errval = errno;
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error: creating a socket: %s" ANSI_DEFAULT_COLOR "\n", strerror(errval));
			continue;
		}

		//when we get here, we created the socket, so we try to connect
		if (connect(socket_fd, i->ai_addr, i->ai_addrlen) < 0) {
			int errval = errno;
			sfwrite(&lock, stderr,  ANSI_ERRORS_COLOR "Error: establishing a connection: %s" ANSI_DEFAULT_COLOR "\n", strerror(errval));
			continue;
		}

		//if we get here, we successfully established a connection; so get out
		break;
	}

	//test case for when we cannot connect
	if (!i) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error: did not establish a connection." ANSI_DEFAULT_COLOR "\n");
		pthread_mutex_destroy(&lock);
		exit(EXIT_FAILURE);
	}
	
	//connection = success; start login protocol
	int login_status = login_protocol(socket_fd, audit_fd, client_name, verbose, new_user);

	//on failure...
	if (login_status < 0 || login_status == 1) {
		if (login_status == -1) {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Did not connect." ANSI_DEFAULT_COLOR "\n");
		}
		else if (login_status == -2) {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Username taken." ANSI_DEFAULT_COLOR "\n");
			client_audit_handler(audit_fd, login_status);
		}
		else if (login_status == -3) {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Invalid password." ANSI_DEFAULT_COLOR "\n");
			client_audit_handler(audit_fd, login_status);
		}
		else if (login_status == -4) {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Invalid username." ANSI_DEFAULT_COLOR "\n");
			client_audit_handler(audit_fd, login_status);
		}
		else if (login_status == -5) {
			sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "An unknown error occurred." ANSI_DEFAULT_COLOR "\n");
			client_audit_handler(audit_fd, login_status);
		}
		logout_handler(socket_fd, verbose);
		pthread_mutex_destroy(&lock);
		exit(EXIT_FAILURE);
	}

	//success: we established a connection; multiplex
	multiplex(socket_fd, audit_fd, verbose);

	//this returns after client calls logout/server disconnects
	close(socket_fd);

	//now close all other sockets and free all mallloc'ed data
	struct chat *ptr = head;
	struct chat *next;
	while (ptr) {
		next = ptr->next;
		kill(ptr->pid, SIGKILL);
		close(ptr->socket_fd);
		free(ptr);
		ptr = next;
	}

	struct fd *fd_ptr = fd_head;
	struct fd *fd_next;
	while (fd_ptr) {
		fd_next = fd_ptr->next;
		close(fd_ptr->fd);
		free(fd_ptr);
		fd_ptr = fd_next;
	}

	//and finally destroy the pthread
	pthread_mutex_destroy(&lock);

	exit(EXIT_SUCCESS);
}

/** This method prints the usage statement for the client. No args/return neccessary.
  * The usage statement is printed exactly from the hw5 specification.
  */
void usage_statement(void) {
	sfwrite(&lock, stdout, "./client [-hcv] [-a FILE] NAME SERVER_IP SERVER_PORT\n\n");
	sfwrite(&lock, stdout, "    -a FILE                Path to the audit log file.\n");
	sfwrite(&lock, stdout, "    -h                     Displays this help menu, and returns EXIT_SUCCESS.\n");
	sfwrite(&lock, stdout, "    -c                     Requests to server to create a new user.\n");
	sfwrite(&lock, stdout, "    -v                     Verbose print all incoming and outgoing protocol verbs & content.\n\n");
	sfwrite(&lock, stdout, "NAME                       This is the username to display when chatting.\n");
	sfwrite(&lock, stdout, "SERVER_IP                  The IP address of the server to connect to.\n");
	sfwrite(&lock, stdout, "SERVER_PORT                The port to connect to.\n");
}

/** This method prints the error messages from the login method
  * to the audit log.
  *
  * @param audit_fd - The socket file descriptor to the audit log
  * @param err_number - Specifies which error occurred
  */
void client_audit_handler(int audit_fd, int err_number) {
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
	strcat(audit_buffer, ", fail, ");

	if (err_number == -2)
		strcat(audit_buffer, ERROR_USERNAME);
	else if (err_number == -3)
		strcat(audit_buffer, ERROR_PASSWORD);
	else if (err_number == -4)
		strcat(audit_buffer, ERROR_NOT_AVAIL);
	else
		strcat(audit_buffer, ERROR_SERVER);

	strcat(audit_buffer, "\n");

	//write it
	flock(audit_fd, LOCK_EX);
	write(audit_fd, audit_buffer, strlen(audit_buffer));
	flock(audit_fd, LOCK_UN);
}