all: client server chat

client:	client.c client_login_multiplex.c client_stdin_handlers.c client_server_handlers.c client_chat_handlers.c sfwrite.c
	gcc -Wall -Werror client.c client_login_multiplex.c client_stdin_handlers.c client_server_handlers.c client_chat_handlers.c sfwrite.c -o $@

chat: chat.c
	gcc -Wall -Werror chat.c -o $@

server: server.c server_functions.c server_handlers.c client_com_handlers.c file_handler.c error_handlers.c sfwrite.c
	gcc -Wall -Werror server.c server_functions.c server_handlers.c client_com_handlers.c file_handler.c error_handlers.c sfwrite.c -o $@ -pthread

