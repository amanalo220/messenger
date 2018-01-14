#include "server.h"

int main(int argc, char *argv[]) {

	v = false;
	int listenfd, connfd, t_count;
	char *port_num, *msg;
	socklen_t clientlen;
	struct sockaddr_storage clientaddr;
	pthread_mutex_t q_lock = PTHREAD_MUTEX_INITIALIZER;
	
	byte_cnt = 0;
	sd = false;
	t_count = 2;
	sem_init(&items_sem, 0, 0);
	user_file = NULL;

	memset(msg_buf, 0, MAX_BUF_SIZE);

	//init pthread_mutex
	if(pthread_mutex_init(&lock, NULL) != 0) {
		fprintf(stderr, ANSI_ERRORS_COLOR "Lock failed to init\n" ANSI_DEFAULT_COLOR);
		exit(EXIT_FAILURE);
	}

	//check the command-line arguments

	//handle the help menu, if there is one.
	int args;
	for (args = 1; args < argc; args++) {
		if (strcmp(argv[args], "-h") == 0) {
			print_help();
			pthread_mutex_destroy(&lock);
			exit(EXIT_SUCCESS);
		}
	}

	if(strcmp(argv[1], "-v") == 0) {
		v = true;
	}

	if(v) {
		if(strcmp(argv[2], "-t") == 0) {
			t_count = atoi(argv[3]);

			port_num = argv[4];
			msg = argv[5];

			if(argc > 6) {
				user_file = init_user_file(argv[6]);
			}
		} else {
			port_num = argv[2];
			msg = argv[3];

			if(argc > 4) {
				user_file = init_user_file(argv[4]);
			}
		}
	} else {
		if(strcmp(argv[1], "-t") == 0) {
			t_count = atoi(argv[2]);

			port_num = argv[3];
			msg = argv[4];

			if(argc > 5) {
				user_file = init_user_file(argv[5]);
			}
		} else {
			port_num = argv[1];
			msg = argv[2];

			if(argc > 3) {
				user_file = init_user_file(argv[3]);
			}
		}
	}

	if(user_file == NULL) {
		user_file = init_user_file("accounts_file.txt");
	} else {
		retrieve_accounts(user_file);
	}

	fprintf(stdout, "t_count: %d\nport: %s\nmessage: %s\n", t_count, port_num, msg);

	pthread_t tid[t_count];

	while(t_count > 0) {
		pthread_create(&tid[t_count - 1], NULL, new_login_thread, &q_lock);
		t_count--;
	}
	
	strcat(msg_buf, msg);

	printf("Currently listening on port %s\n", port_num);

	if((listenfd = open_listenfd(port_num)) < 0) {
		sfwrite(&lock, stderr, ANSI_ERRORS_COLOR "Error: could not open port." ANSI_DEFAULT_COLOR "\n");
		pthread_mutex_destroy(&lock);
		exit(EXIT_FAILURE);
	}

	FD_ZERO(&read_set);
	FD_SET(STDIN_FILENO, &read_set);
	FD_SET(listenfd, &read_set);

	num_of_fd = listenfd + 1;
	
	while(1) {

		ready_set = read_set;
		select(listenfd+1, &ready_set, NULL, NULL, NULL);

		if(FD_ISSET(STDIN_FILENO, &ready_set)) {
			parse_scommand();

			if(sd) {
				sfwrite(&lock, stdout, "server shut down\n");
				break;					//breaks loop and shuts down server
			}
		}

		if(FD_ISSET(listenfd, &ready_set)) {
			clientlen = sizeof(struct sockaddr_storage);
			connfd = accept(listenfd, (SA *)&clientaddr, &clientlen);

			pthread_mutex_lock(&q_lock);
			login_enqueue(connfd, &items_sem);
			pthread_mutex_unlock(&q_lock);

			// pthread_create(&tid, NULL, login_thread, connfdp);
		}

	}

	pthread_mutex_destroy(&lock);
	close_user_file(user_file);
	exit(EXIT_SUCCESS);
}