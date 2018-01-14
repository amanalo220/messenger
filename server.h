#ifndef SERVER_H
#define SERVER_H

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/file.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>
#include "sfwrite.h"

//constants
#define MAX_BUF_SIZE 8192
#define MAX_INPUT 8192
#define MAX_USER_NAME_SIZE 32
#define MAX_PASSWORD_SIZE 32

#define WOLFIE_PROTOCOL "WOLFIE \r\n\r\n"
#define EIFLOW_PROTOCOL "EIFLOW \r\n\r\n"
#define CLIENT_LOGIN "IAM"
#define SERVER_LOGIN "HI"
#define CLIENT_LOGOUT "BYE"
#define CLIENT_LISTU "LISTU"
#define CLIENT_TIME "TIME"
#define SERVER_TIME "EMIT"
#define SERVER_UTSIL "UTSIL"
#define CLIENT_NEW_USER "IAMNEW"
#define SERVER_NEW_USER "HINEW"
#define CLIENT_NEW_PASS "NEWPASS"
#define SERVER_NEW_PASS "SSAPWEN"
#define SERVER_MSG "MOTD"
#define SERVER_AUTHENTICATE "AUTH"
#define CLIENT_PASSWORD "PASS"
#define SERVER_PASSWORD "SSAP"
#define MSG "MSG"
#define UOFF "UOFF"

#define END_PROTOCOL " \r\n\r\n"
#define USER_PROTOCOL " \r\n"

#define ERROR_USERNAME "ERR 00 "
#define ERROR_NOT_AVAIL "ERR 01 "
#define ERROR_PASSWORD "ERR 02 "
#define ERROR_SERVER "ERR 100 "

#define ANSI_VERBOSE_COLOR "\x1B[1;34m"
#define ANSI_ERRORS_COLOR "\x1B[1;31m"
#define ANSI_DEFAULT_COLOR "\x1B[0m"

#define VERBOSE_OUT 1
#define VERBOSE_IN 2
#define OUT_STRING "Outgoing: "
#define IN_STRING "Incoming: "

//structs
struct client {
	int fd;
	time_t start_time;
	struct client *prev;
	struct client *next;
	char user_name[MAX_USER_NAME_SIZE];
	char *buf_ptr;
	char buf[MAX_BUF_SIZE];
};

struct account {
	char user_name[MAX_USER_NAME_SIZE];
	char password[MAX_PASSWORD_SIZE];
	struct account *prev;
	struct account *next;
};

struct login {
	int fd;
	struct login *next;
};

typedef struct sockaddr SA;

//global variables
int byte_cnt;
struct client *client_head;				//start of client linked list
struct account *account_head;			//start of account linked list
bool com_thread_set;
bool sd;								//shutdown
char msg_buf[MAX_BUF_SIZE];
fd_set read_set, ready_set;				//fd_sets
int num_of_fd;							//number of file descriptors to listen on
bool v;									//verbose
FILE *user_file;						//userfile
pthread_mutex_t lock;
struct login *login_head;				//head of login queue
struct login *login_tail;				//tail of login queue
sem_t items_sem;

//server functions
void print_help();
int open_listenfd(char*);
int add_client(int, char*);
void remove_client(struct client*);
void parse_scommand();
void send_to_client(int, char[]);
void parse_ccommands(int);
void *new_login_thread(void *vargp);
void *com_thread(void *vargp);
void create_account(char*, char*);
void verbose(char*, int);
int findFD(char*);
void login_enqueue(int, sem_t*);
int login_dequeue();

//server command handlers
void users_cmd_handler();
void help_cmd_handler();
void shutdown_cmd_handler();
void accounts_cmd_handler();
void print_client(struct client*);
void print_account(struct account*);

//client communication handlers
int login_protocol(int);
int wolfie_protocol_handler(int);
int username_check(char*);
void client_logout(int);
void client_listu(int);
void client_time(int);
void client_msg(int, char*);

//user file functions
FILE *init_user_file(char*);
char *get_user_info(FILE*, int);
int add_user_info(FILE*, char*, char*);
void close_user_file(FILE*);
void retrieve_accounts(FILE*);

//error handlers
void err_user_taken(int, char*);
void err_bad_password(int);
void err_user_not_avail(int);
void err_server(int);

#endif
