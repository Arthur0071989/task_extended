#include "user_list.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>


//...
extern int errno;
//...
// printf("%s/n",strerror(errno));


struct ul_ctx{
	size_t size;
	user ** users;
};

user_list * create_ul(size_t size){
	user_list * res = malloc(sizeof(user_list));
	res->ctx = malloc(sizeof(ul_ctx));
	res->ctx->size = size;
	res->ctx->users = malloc(size* sizeof(users*));
	memset(res->ctx->users, 0, (size*sizeof(user*)));
	//set metohod pointers here
	return res;
}

void destroy_ul(user_list *ul){

	size_t i = 0;
	for(; i<ul->ctx->size; ++i){
		if(ul->ctx->users[i]){
			free(ul->ctx->users[i]->nick);
			free(ul->ctx->users[i]);
			ul->ctx->users[i] = 0;
		}
	}
	free(ul->ctx);
	free(ul);
}

/* metoda znajdowania */
static user * find_by_fd(user_list *ul, int fd){
	size_t i=0;
	for(;i<ul->ctx->size; ++i){
		if(ul->ctx->users[i] && (ul->ctx->users[i]->fd == fd)){
			return ul->ctx->users[i];
		}
	}
	return 0; //return null if not found
}

static int add(user_list *ul, user *u){
	if(find_by_fd(ul, u->fd)!=0)
		return 1; //user already exists

	for(; i<ul->ctx->size; ++i){
		if(ul->ctx->users[i] == 0){
			ul->ctx->users[i] = u;
			return 0;
		}
	}
	return 1;  //there is no place
}

