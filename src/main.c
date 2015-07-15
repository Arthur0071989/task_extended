#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include "user_list.h"

#define BUFF_SIZE 1024

user_list* USER_LIST;

/* wstawka z funkcja odczytu protokolu */
size_t receive_msg(char** msg, int fd);
int server_service(int srv_fd, int epoll_fd);
void client_service(int cli_fd, uint32_t events, int epoll_fd);
void clear_client(int cli_fd, int epoll_fd);
int handle_login(const char* msg, size_t len, int fd);
int handle_userlist(const char* msg, size_t len, int fd);
int send_acknack(int fd, int error_code, const char* error_msg);
int send_msg(int fd, const char* msg, size_t msg_len);

int main(int argc, const char *argv[])
{
    int i = 0;
    size_t client_capacity = 100;
    size_t total_capacity = 101; //+1 extra for server socket

    int srv_fd = -1;
    struct sockaddr_in srv_addr;

    int epoll_fd = -1;
    struct epoll_event e, *es;

    if (argc == 2) {
    	client_capacity = atoi(argv[1]); //read capacity from program argument list
    	total_capacity = client_capacity + 1;
    }

    USER_LIST = create_ul(client_capacity);

    memset(&srv_addr, 0, sizeof(srv_addr));
    memset(&e, 0, sizeof(e));

    es = malloc((total_capacity) * sizeof(struct epoll_event));

    srv_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (srv_fd < 0) {
        printf("Cannot create socket\n");
        return 1;
    }

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(5558);
    if (bind(srv_fd, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) < 0) {
        printf("Cannot bind socket\n");
        close(srv_fd);
        return 1;
    }

    if (listen(srv_fd, total_capacity) < 0) {
        printf("Cannot listen\n");
        close(srv_fd);
        return 1;
    }

    epoll_fd = epoll_create(total_capacity);
    if (epoll_fd < 0) {
        printf("Cannot create epoll\n");
        close(srv_fd);
        return 1;
    }

    e.events = EPOLLIN;
    e.data.fd = srv_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, srv_fd, &e) < 0) {
        printf("Cannot add server socket to epoll\n");
        close(epoll_fd);
        close(srv_fd);
        return 1;
    }

    for(;;) {
        i = epoll_wait(epoll_fd, es, total_capacity, -1);
        if (i < 0) {
            printf("Cannot wait for events\n");
            close(epoll_fd);
            close(srv_fd);
            return 1;
        }

        for (--i; i > -1; --i) {
            if (es[i].data.fd == srv_fd) {
                if (server_service(srv_fd, epoll_fd)) {
                	return 1;
                }
            } else {
            	client_service(es[i].data.fd, es[i].events, epoll_fd);
            }
        }
    }

	return 0;
}

int server_service(int srv_fd, int epoll_fd)
{
	int cli_fd = -1;
	struct sockaddr_in cli_addr;
	socklen_t cli_addr_len = sizeof(struct sockaddr_in);
	struct epoll_event e;

	memset(&cli_addr, 0, sizeof(cli_addr));
	memset(&e, 0, sizeof(e));

	cli_fd = accept(srv_fd, (struct sockaddr*) &cli_addr, &cli_addr_len);
	if (cli_fd < 0) {
		printf("Cannot accept client\n");
		close(epoll_fd);
		close(srv_fd);
		return 1;
	}

	e.events = EPOLLIN;
	e.data.fd = cli_fd;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cli_fd, &e) < 0) {
		printf("Cannot accept client\n");
		close(epoll_fd);
		close(srv_fd);
		return 1;

	}

	return 0;
}

void client_service(int cli_fd, uint32_t events, int epoll_fd)
{
	char *msg = 0;
	size_t len = 0;

	int if_clear_client = 1;

	if (events & EPOLLIN) {
		/* odczyt wiadomosci */
		len = receive_msg(&msg, cli_fd);
		if(len > 0) {
			if(msg[0] =='2') {
				printf("ok");
				if_clear_client = handle_login(msg, len, cli_fd);
			}
			else if(msg[0] == '6') {
				if_clear_client = handle_userlist(msg, len, cli_fd);
			}
		}
	}

	if (if_clear_client) {
		clear_client(cli_fd, epoll_fd);
	}
}

void clear_client(int cli_fd, int epoll_fd)
{
	//TODO remove user from the list
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cli_fd, 0);
	close(cli_fd);
}

size_t receive_msg(char** msg, int fd)
{
	size_t len = 0;
	read(fd, &len, sizeof(size_t));
	(*msg) = malloc((len+1)* sizeof(char));
	(*msg)[len] = 0;
	read(fd, (*msg), len);
	return len;
}

int handle_login(const char* msg, size_t len, int fd)
{
	int result = 1;
	user* u = 0;

	/* read nick from message */
	size_t nick_len = strlen(msg+2);
	char* nick = malloc((nick_len+1) * sizeof(char));
	strcpy(nick, msg+2);
	u = malloc(sizeof(user));
	u->fd = fd;
	u->nick = nick;

	/* add user to user list */
	if (USER_LIST->add(USER_LIST, u) == 0) {
		/* send ack "1.0" if user added, send nack otherwise */
		result = send_acknack(fd, 0, 0);
	} else {
		result = send_acknack(fd, 1, "Cannot add user, nick already exists or server is full");
	}

	return result;
}

int handle_userlist(const char* msg, size_t len, int fd)
{
	char* msg_ulr = 0;
	size_t msg_ulr_len = 0;

	//TODO
	//		* get users from user list
	//		* prepare message user list reply
	//		* send message user list reply

	return send_msg(fd, msg_ulr, msg_ulr_len);
}

int send_acknack(int fd, int error_code, const char* error_msg)
{
	char* msg = 0;
	size_t msg_len = 0;

	if (error_code) {
		msg_len = strlen(error_msg) + 4;
		msg = malloc((msg_len + 1) * sizeof(char));
		msg[0] = '1';
		msg[1] = '.';
		msg[2] = '1';
		msg[3] = '.';
		strcpy(msg+5, error_msg);
	} else {
		msg = "1.0";
		msg_len = 3;
	}

	return send_msg(fd, msg, msg_len);
}

int send_msg(int fd, const char* msg, size_t msg_len)
{
	if (write(fd, &msg_len, sizeof(size_t)) != sizeof(size_t))
		return 1;

	if (write(fd, msg, msg_len) != msg_len)
		return 1;

	return 0;
}
