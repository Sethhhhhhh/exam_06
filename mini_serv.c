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

/* structure client qui contient l'id du client ainsi qu'une variable pour stocker ses messages. */
typedef struct s_client {
	int		id;
	char	msg[100000];
}				t_client;

/* creer un tableau de clients (1024). */
t_client	clients[1024];

/* deux buffers pour l'ecriture et la lecture. */
char	 	read_buf[110000], write_buf[110000];

void	_err() {
	/* ecrit un message sur la sortie d'erreur. */
	write(2, "Fatal error\n", 12);
	exit(1);
}

void _send(int socket, int max, fd_set* writefds) {
	/* boucle sur chaque socket du fd set. */
    for (int i = 0; i <= max; i++)
        if (FD_ISSET(i, writefds) && i != socket) /* regarde si le socket est dans le fd set. */
            send(i, write_buf, strlen(write_buf), 0); /* envoie au client le message dans le buffer write. */
}

int main(int ac, char** av) {
	/* initalisation des variables. */
	int			server_socket, client_socket, octets, port, id = 0, max = 0;
	fd_set		current, writefds, readfds;
	
	/* si le nombre d'argument n'est egal a 2, quitter le programme avec un code d'erreur egal a 1. */
	if (ac != 2) {
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}

	/* transformer le port de chaine de caractere a un nombre. */
	port = atoi(av[1]);

	/* intialise tous les structures clients. */
	bzero(&clients, sizeof(clients));
	
	/* clear le fd set current. */
	FD_ZERO(&current);

	/* creer le socket et retourne une erreur si la creation n'a pas fonctionne. */
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		_err();

	max = server_socket;
	
	/* ajoute le socket du serveur au fd set. */
	FD_SET(server_socket, &current);

	/* Setup de l'addresse sur la quelle bind le socket du serveur. */
	struct sockaddr_in	addr;
	socklen_t addr_len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(2130706433);
	addr.sin_port = htons(port);

	/* bind le socket du serveur sur l'addresse passe en parametre et met le socket en ecoute. renvoie une erreur sur une des deux etapes ne fonctionne pas. */
	if (bind(server_socket, (const struct sockaddr *)&addr, sizeof(addr)) == -1 || listen(server_socket, 128) == -1)
		_err();
	
	/* boucle infini pour toujours reste en ecoute. */
	while (1) {
		readfds = writefds = current;

		/* surveilles tous les descripteurs de fichier dans le socket serveur. */
		if (select(max + 1, &readfds, &writefds, NULL, NULL) == -1)
			continue;

		/* boucle sur tous les sockets jusqu'au socket maximum. */
		for (int socket = 0; socket <= max; socket++) {

			/* regarde si le socket est dans le fd set et regarde si le socket est egal au socket serveur. */
			if (FD_ISSET(socket, &readfds)) {
				if (socket == server_socket) {
					
					/* accepte la premiere connexion dans la file d'attente. Si il echoue le programme passe au prochain socket. */
					if ((client_socket = accept(server_socket, (struct sockaddr *)&addr, &addr_len)) == -1)
						continue;
					
					/* si l'identifiant du socket client est plus grand que l'identifiant du socket maximum il met maximum a la valeur du socket client. */
					if (client_socket > max) max = client_socket; 
					
					/* defini l'id du client a id et incremente id. */
					clients[client_socket].id = id++;

					/* ajoute le socket client dans le fd set. */
					FD_SET(client_socket, &current);

					/* ecrit dans le buffer d'ecriture qu'un client vient d'arriver. */
					sprintf(write_buf, "server: client %d just arrived\n", clients[client_socket].id);
					
					/* envoie a tous les clients ce qui est dans le buffer d'ecriture. */
					_send(client_socket, max, &writefds);
				} else {
					/* regarde si le nombre d'octets est inferieur ou egal a 0. */
					if ((octets = recv(socket, read_buf, 110000, 0)) <= 0) {
						/* envoie qu'un joueur est partie dans le buffer write. */
						sprintf(write_buf, "server: client %d just left\n", clients[socket].id);

						/* envoie ce qu'il y a dans le buffer write a tous les clients. */
						_send(socket, max, &writefds);

						/* supprime le socket du client qui est partie du fd set. */
						FD_CLR(socket, &current);

						/* ferme le socket. */
						close(socket);
					} else {
						/* Boucle sur chaque caractere du message du client. */
						for (int k = 0, v = strlen(clients[socket].msg); k < octets; k++, v++) {
							/* copie caractere par caractere dans la structure du client. */
							clients[socket].msg[v] = read_buf[k];

							/* check si le caractere est un retour a la ligne. */
							if (clients[socket].msg[v] == '\n') {
								
								/* remplace le caractere par un \0. */
								clients[socket].msg[v] = '\0';
								
								/* copie le message du client dans le buffer write. */
								sprintf(write_buf, "client %d: %s\n", clients[socket].id, clients[socket].msg);
								
								/* transmet le message a tous les clients. */
								_send(socket, max, &writefds);
								
								/* remplace tous les caracteres de msg dans la structure du client par des 0. */
								bzero(&clients[socket].msg, strlen(clients[socket].msg));
							}
						}
					}
				}
				break;
			}
		}
	}
	return 0;
}