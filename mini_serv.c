#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>

typedef struct s_client {
	int		id;
	char	msg[100000];
}				t_client;

t_client	clients[1024];
char	 	read_buf[110000], write_buf[110000];

void	_err() {
	write(2, "Fatal error\n", 12);
	exit(1);
}

void _send(int socket, int max, fd_set* in) {
    for (int i = 0; i <= max; i++)
        if (FD_ISSET(i, in) && i != socket)
            send(i, write_buf, strlen(write_buf), 0);
}

int main(int ac, char** av) {
	int			server_socket, client_socket, port, id = 0, max = 0;
	fd_set		current, out, in;
	
	if (ac != 2) {
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	port = atoi(av[1]);
	bzero(&clients, sizeof(clients));
	FD_ZERO(&current);

	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		_err();
	max = server_socket;
	FD_SET(server_socket, &current);

	struct sockaddr_in	addr;
	socklen_t addr_len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(2130706433);
	addr.sin_port = htons(port);

	if (bind(server_socket, (const struct sockaddr *)&addr, sizeof(addr)) == -1 || listen(server_socket, 128) == -1)
		_err();
	
	while (1) {
		out = in = current;

		if (select(max + 1, &out, &in, NULL, NULL) == -1)
			continue;
		
		for (int socket = 0; socket <= max; socket++) {
			if (FD_ISSET(socket, &out) && socket == server_socket) {
				if ((client_socket = accept(server_socket, (struct sockaddr *)&addr, &addr_len)) == -1)
					continue;
				
				if (client_socket > max) max = client_socket; 
				clients[client_socket].id = id++;

				FD_SET(client_socket, &current);
			 	sprintf(write_buf, "server: client %d just arrived\n", clients[client_socket].id);
				_send(client_socket, max, &in);

				break;
			}
			
			if (FD_ISSET(socket, &out) && socket != server_socket) {
				int octets = recv(socket, read_buf, 110000, 0);
				
				if (octets <= 0) {
					sprintf(write_buf, "server: client %d just left\n", clients[socket].id);
					
					_send(socket, max, &in);
					FD_CLR(socket, &current);

					close(socket);
				} else {
					for (int k = 0, v = strlen(clients[socket].msg); k < octets; k++, v++) {
						clients[socket].msg[v] = read_buf[k];

						if (clients[socket].msg[v] == '\n') {
							clients[socket].msg[v] = '\0';
							sprintf(write_buf, "client %d: %s\n", clients[socket].id, clients[socket].msg);
							_send(socket, max, &in);
							bzero(&clients[socket].msg, strlen(clients[socket].msg));
							v = -1;
						}
					}
				}
				break;
			}
		}
	}
	return 0;
}