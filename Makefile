server: server.c
	cc -g -Wall -Wextra -Werror server.c -o server

client: client.c
	cc -g -Wall -Wextra -Werror client.c -o client
