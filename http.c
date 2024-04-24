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
int connfd;

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

char* http_head(char* headers_str, char* header){
    if(headers_str == NULL){
        char* nheaders_str = (char*)malloc(sizeof(char)*strlen(header)+2);
        *nheaders_str = '\0';
        strncat(nheaders_str, header, sizeof(char)*strlen(header));
        return nheaders_str;
    }

    //printf("%ld\n", (sizeof(char)*strlen(headers_str))+(sizeof(char)*strlen(header))+2);
    char* nheaders_str = (char*)malloc((sizeof(char)*strlen(headers_str))+(sizeof(char)*strlen(header))+2);
    *nheaders_str = '\0';
    strncat(nheaders_str, headers_str, sizeof(char)*strlen(headers_str));
    strncat(nheaders_str, "\n",sizeof(char));
    strncat(nheaders_str, header, sizeof(char)*strlen(header));
    free(headers_str);
    return nheaders_str;
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

    // 404'd 
    if(file == NULL){
        char* head = "HTTP/1.1 404 OK\nServer: httpc\nContent-Type: text/html\n\n\
        <h1 style='text-align: center;'>404 Not Found</h1>\
        <hr/>\
        <p style='text-align: center;'>httpc dev</p>";

        char* res = (char*)malloc((sizeof(char)*strlen(head))+1);
        *res = '\0';
        strncat(res, head, sizeof(char)*strlen(head));
        return res;
    }

    char head[] = "HTTP/1.1 200 OK\n";

    // temporary solution to handle html, js, json, css and png
    char* headers = NULL;
    headers = http_head(headers, "Server: httpc");
    
    char* tmp = fpath;
    fpath = (char*) malloc(strlen(tmp)+1);
    *fpath='\0';
    strncat(fpath, tmp, sizeof(char)*strlen(tmp)); 

    char* ext = strtok(fpath, ".");
    ext = strtok(NULL, ".");

    // temporary solution :D
    if(ext == NULL){
        headers = http_head(headers, "Content-Type: text/plain");
    }
    if(strcmp(ext, "html") == 0){
       headers = http_head(headers, "Content-Type: text/html"); 
    }
    else if(strcmp(ext, "js") == 0){
        headers = http_head(headers, "Content-Type: text/javascript");
    }
    else if(strcmp(ext, "css") == 0){
        headers = http_head(headers, "Content-Type: text/css");
    }
    else if(strcmp(ext, "json") == 0){
        headers = http_head(headers, "Content-Type: application/json");
    }
    else if(strcmp(ext, "png") == 0){
        headers = http_head(headers, "Content-Type: image/png");
    } else headers = http_head(headers, "Content-Type: text/plain");
    // endsection

    char* res = (char*)malloc((sizeof(char)*sizeof(head))+(sizeof(char)*strlen(headers))+(sizeof(char)*strlen(file))+8);
    *res = '\0';
    strncat(res, head, sizeof(char)*sizeof(head));
    strncat(res, headers, strlen(headers));
    strncat(res, "\n\n", sizeof(char)*2);
    strncat(res, file, strlen(file));

    free(headers);
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

    // TODO: add control in order to avoid crashing the server
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
    close(connfd);
    close(sfd);
}