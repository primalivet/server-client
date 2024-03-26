server: server.c
	cc -g -Wall -Wextra -Werror server.c -o server

client: client.c
	cc -g -Wall -Wextra -Werror client.c -o client

clean:
	rm -rf server.dSYM server client.dSYM client
