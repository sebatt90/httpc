#include "sys/socket.h"
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "http.h"

extern inline void catch_error(int condition, char* msg){
    if(condition == -1){
        fprintf(stderr, "(ERROR) %s\n", msg);
        exit(-1);
    }
}

char* req_error(){
    fprintf(stderr, "(ERROR) HTTP Request Error");
    return "HTTP/1.1 200 OK\nServer:httpc\n\n<h1>HTTP Request Error</h1>";
}

int sfd;

char* folder;

char* openfile(const char* filepath){
    char* fpath = (char*) malloc(strlen(filepath)+strlen(folder)+1);

    strncpy(fpath, folder, strlen(folder));
    strncat(fpath, filepath, strlen(filepath)+1);

    FILE* fd;
    if(!(fd=fopen(fpath, "r"))) return 0;

    fseek(fd, 0, SEEK_END);
    long fsize = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    char* str = (char*) malloc(fsize+1);
    fread(str, fsize, 1, fd);
    fclose(fd);
    str[fsize] = '\0';

    return str;
}


char* process_req(char* req){
    // Example: GET / HTTP/1.1
    char* method = strtok(req, " ");
    if(method == NULL) return req_error();
    char* route = strtok(NULL, " ");
    if(route == NULL) return req_error();
    char* http_version = strtok(NULL, " ");
    if(http_version == NULL) return req_error();

    char* fpath = (strcmp(route, "/")==0) ? "/index.html" : route;
    

    char* file = openfile(fpath);

    if(file == NULL){
        char* head = "HTTP/1.1 400 OK\nServer: httpc\nContent-Type: text/html\n\n";
        char* res = (char*)malloc((sizeof(char)*sizeof(head))+(sizeof(char)*sizeof("404 Content Not Found")));
        strcpy(res, head);
        strcat(res, "404 Content Not Found\0");
        return res;
    }

    char head[] = "HTTP/1.1 200 OK\nServer: httpc\nContent-Type: text/html\n\n";

    // TODO: handle multiple file types other than html...

    char* res = (char*)malloc((sizeof(char)*sizeof(head))+(sizeof(char)*strlen(file))+2);
    strncpy(res, head, sizeof(head));
    strncat(res, file, strlen(file));

    return res;
}

void http_init(unsigned short port, const char* content){

    // getting our server folder
    folder = (char*) malloc(strlen(content)+1);
    strncpy(folder, content, strlen(content)+1);

    sfd = socket(AF_INET, SOCK_STREAM, 0);

    // error while opening socket
    catch_error(sfd, "An error occured while creating the TCP socket!");

    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    catch_error(bind(sfd, (struct sockaddr *) &addr, sizeof(addr)), "Error while binding the TCP socket!");

    int backlog = 10;

    // error while listening
    catch_error(listen(sfd, backlog), "An error has occured while trying to listen for incoming connections!");

    printf("(OK) HTTP Server is listening on port %d\n", port);
}

void http_accept() {
    int connfd;
    struct sockaddr_in cliaddr;
    unsigned int cliaddr_len = sizeof(cliaddr);

    connfd = accept(sfd, (struct sockaddr *) &cliaddr, &cliaddr_len);
    if (connfd == -1) {
        printf("A connection error has occured!\n");
        return;
    }

    char req[1024];
    int blen;
    // read req
    blen = read(connfd, req, sizeof(req));
    
    if(blen == -1)
    {
        // handle
        printf("Invalid Request!\n");
        return;
    }


    char* res = process_req(req);

    if(write(connfd, res, strlen(res)+1) == -1){
        // error
        printf( "An error has occured while writing to HTTP Client\n");
        return;
    }
    free(res);
    close(connfd);
}

void http_close(){
    close(sfd);
}