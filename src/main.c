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


/* wstawka z funkcja odczytu protokolu */
size_t receive_msg(char** msg, int fd)
{
	size_t len = 0;
	read(fd, &len, sizeof(size_t));
	(*msg) = malloc((len+1)* sizeof(char));
	(*msg)[len] = 0;
	read(fd, (*msg), len);
	return len;
}

void handle_login(const char* msg, size_t len, int fd);
void handle_userlist(const char* msg, size_t len, int fd);

int main(int argc, const char *argv[])
{
    int i = 0;

    int srv_fd = -1;
    int cli_fd = -1;
    int epoll_fd = -1;

    struct sockaddr_in srv_addr;
    struct sockaddr_in cli_addr;
    socklen_t cli_addr_len;
    struct epoll_event e, es[2];

    memset(&srv_addr, 0, sizeof(srv_addr));
    memset(&cli_addr, 0, sizeof(cli_addr));
    memset(&e, 0, sizeof(e));

    srv_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (srv_fd < 0) {
        printf("Cannot create socket\n");
        return 1;
    }

    srv_addr.sin_family = AF_INET;
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    srv_addr.sin_port = htons(5557);
    if (bind(srv_fd, (struct sockaddr*) &srv_addr, sizeof(srv_addr)) < 0) {
        printf("Cannot bind socket\n");
        close(srv_fd);
        return 1;
    }

    if (listen(srv_fd, 1) < 0) {
        printf("Cannot listen\n");
        close(srv_fd);
        return 1;
    }

    epoll_fd = epoll_create(2);
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
        i = epoll_wait(epoll_fd, es, 2, -1);
        if (i < 0) {
            printf("Cannot wait for events\n");
            close(epoll_fd);
            close(srv_fd);
            return 1;
        }

        for (--i; i > -1; --i) {
            if (es[i].data.fd == srv_fd) {
                cli_fd = accept(srv_fd, (struct sockaddr*) &cli_addr, &cli_addr_len);
                if (cli_fd < 0) {
                    printf("Cannot accept client\n");
                    close(epoll_fd);
                    close(srv_fd);
                    return 1;
                }

                e.data.fd = cli_fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cli_fd, &e) < 0) {
                    printf("Cannot accept client\n");
                    close(epoll_fd);
                    close(srv_fd);
                    return 1;

                }
            } else {
            	/* odczyt wiadomosci */
                if (es[i].events & EPOLLIN) {
                	char *msg = 0;
                	size_t len = receive_msg(&msg, es[i].data.fd);
                	if(len > 0) {
                		if(msg[0] =='2')
                			handle_login(msg, len, es[i].data.fd);
                		else if(msg[0] == '6')
                			handle_userlist(msg, len, es[i].data.fd);
                	}
                }
            }
        }
    }

	return 0;
}

void handle_login(const char* msg, size_t len, int fd)
{
	//TODO:
	//		* read nick from message
	//		* add user to user list
	//		* send ack "1.0" if user added, send nack otherwise
	read(fd, (*msg), len);
	user_list *ul = create_ul(2);
	ul->add(ul, 0);

	//add(user_list *ul, user *u);

}

void handle_userlist(const char* msg, size_t len, int fd)
{
	//TODO

}
