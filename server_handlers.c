#include "server.h"

void users_cmd_handler() {
	struct client *c = client_head;

	pthread_mutex_lock(&lock);

	while(c != NULL) {
		print_client(c);
		c = c->next;
	}

	pthread_mutex_unlock(&lock);
}

void help_cmd_handler() {
	sfwrite(&lock, stdout, "Available commands for server:\n\n");
	sfwrite(&lock, stdout, "/users          prints all of the users that are currently logged in\n");
	sfwrite(&lock, stdout, "/help           lists all commands that the server accepts\n");
	sfwrite(&lock, stdout, "/accts          prints out each account's information\n");
	sfwrite(&lock, stdout, "/shutdown       shuts down the server\n\n");
}

void shutdown_cmd_handler() {
	
	struct client *c = client_head;
	struct client *next;
	struct account *a = account_head;
	struct account *n;

	while(c != NULL) {
		next = c->next;
		close(c->fd);
		free(c);
		c = next;
	}

	while(a != NULL) {
		n = a->next;
		free(a);
		a = n;
	}

	sd = true;
}

void accounts_cmd_handler() {
	struct account *a = account_head;

	while(a != NULL) {
		print_account(a);
		a = a->next;
	}
}

void print_client(struct client *c) {
	sfwrite(&lock, stdout, "Client:\n");
	sfwrite(&lock, stdout, "fd:             %d\n", c->fd);
	sfwrite(&lock, stdout, "username:       %s\n\n", c->user_name);
}

void print_account(struct account *a) {
	sfwrite(&lock, stdout, "Account:\n");
	sfwrite(&lock, stdout, "username:       %s\n", a->user_name);
	sfwrite(&lock, stdout, "password:       %s\n\n", a->password);
}