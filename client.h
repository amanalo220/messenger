#ifndef CLIENT_H
#define CLIENT_H

#include "sfwrite.h"

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

//client prototype
void usage_statement(void);
void client_audit_handler(int, int);

//login and multiplexing prototypes
int login_protocol(int, int, char *, bool, bool);
int login_new_user_protocol(int, int, char *, bool);
void multiplex(int, int, bool);
void add_fd(int, char *);
void remove_fd(int);
void login_audit_handler(int, char *);

//client stdin commands
void parse_stdin_commands(int, int, bool);
void time_handler(int, bool);
void help_handler(void);
void logout_handler(int, bool);
void listu_handler(int, bool);
int chat_handler(char *, int, bool);
void audit_handler();
void stdin_audit_handler(int, char *, bool);

//client server commands
int parse_server_commands(int, int, bool);
void emit_handler(char *, bool);
int bye_handler(char *, int, bool);
void users_handler(char *, bool);
void messages_handler(char *, int, bool);
void error_handler(char *, int, bool);
void uoff_handler(char *, bool);
void server_audit_handler(int, char *);

//client chat commands
void parse_chat_commands(int, int, char *);
void chat_message_handler(char *, char *, int, bool);
void print_message_to_chat(char *, int, bool);
void end_chat_term(char *);
void chat_audit_handler(int, char *, char *, bool);

//global variables
char *client_name;
char *ip;
char *port;
char *audit_file;
bool intentional;
pthread_mutex_t lock;

//chat structs
struct chat {
	char other_person[100];
	int socket_fd;
	int pid;
	struct chat *next;
};

struct fd {
	int fd;
	struct fd *next;
};

extern struct chat *head;
extern struct fd *fd_head;

//constants
#define MAX_BUF_SIZE 8192
#define MAX_INPUT 8192

#define CLIENT_WOLFIE "WOLFIE"
#define CLIENT_IAM "IAM "
#define CLIENT_IAMNEW "IAMNEW "
#define CLIENT_NEWPASS "NEWPASS "
#define CLIENT_PASS "PASS "
#define CLIENT_TIME "TIME"
#define CLIENT_SERVER_BYE "BYE"
#define CLIENT_LISTU "LISTU"
#define CLIENT_CHAT "MSG "
#define CLIENT_SERVER_ERROR "ERR "

#define SERVER_WOLFIE "EIFLOW"
#define SERVER_HI "HI "
#define SERVER_AUTH "AUTH "
#define SERVER_HINEW "HINEW "
#define SERVER_NEWPASS "SSAPWEN"
#define SERVER_PASS "SSAP"
#define SERVER_MOTD "MOTD "
#define SERVER_TIME "EMIT "
#define SERVER_LISTU "UTSIL "
#define SERVER_UOFF "UOFF "

#define END_SYMBOLS " \r\n\r\n"
#define DELIMITER " \r\n "

#define ERROR_USERNAME "ERR 00 USER NAME TAKEN"
#define ERROR_NOT_AVAIL "ERR 01 USER NOT AVAILABLE"
#define ERROR_PASSWORD "ERR 02 BAD PASSWORD"
#define ERROR_SERVER "ERR 100 INTERNAL SERVER ERROR"

#define ANSI_VERBOSE_COLOR "\x1B[1;34m"
#define ANSI_ERRORS_COLOR "\x1B[1;31m"
#define ANSI_DEFAULT_COLOR "\x1B[0m"

#endif