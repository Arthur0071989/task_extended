#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

#define BUFF_SIZE 1024




/* struktura uzytkownikow */
struct user {
	int fd;
	char *nick;
};
/* wstawka z funkcja odczytu protokolu */
size_t receive_msg(char** msg, int fd){
	size_t len = 0;
	read(fd, &len, sizeof(size_t));
	(*msg) = malloc((len+1)* sizeof(char));
	(*msg)[len] = 0;
	read(fd, (*msg), len);
	return len;
}


int main(int argc, const char *argv[])
{
    int i = 0;
    char buff[BUFF_SIZE];
    ssize_t msg_len = 0;

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
            	es[i].data.f;
            	char *msg = 0;
            	size_t len = receive_msg(&msg, es[i].data.f);
            	if(len > 0)
            		if(msg[0] =='2')
            			handle_login(msg, len, es[i].data.f);
            		else if(msg[0] == '6')
            			handle_userlist(msg, len, es[i].data.f);
                if (es[i].events & EPOLLIN) {
                    msg_len = read(cli_fd, buff, BUFF_SIZE);
                    if (msg_len > 0) {
                        write(cli_fd, buff, msg_len);
                    }
                    close(cli_fd);
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cli_fd, &e);
                    cli_fd = -1;
                }
            }
        }
    }

	return 0;
}

