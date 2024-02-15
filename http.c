#include "sys/types.h"
#include "sys/socket.h"
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "http.h"

int sfd;

char* folder;

void http_head(const char* str, char** method, char** location){
    int i = 0;
    int sstr = 0;
    
    char* buf = (char*) calloc(sizeof(str),sizeof(char));

    char** point[3] = {method, location};

    while (*(str+i) != '\n') {
        //printf("%c\n", str[i]);
        char ch = *(str+i);
        i++;
        if(ch == ' '){
            char** d = point[sstr];
            *(d) = (char*) malloc(strlen(buf)+1);
            strncpy(*d, buf, strlen(buf)+1);

            // clear
            memset(buf, 0, sizeof(str));
            sstr++;
            continue;
        }

        strncat(buf, &ch, sizeof(char));
        
    }

    free(buf);
}

char* get_src(const char* filepath){
    char* res_body;
    char* fpath = (char*) malloc(strlen(filepath)+strlen(folder)+1);

    strncpy(fpath, folder, strlen(folder));
    strncat(fpath, filepath, strlen(filepath)+1);

    printf("path: %s\n",fpath);
    
    FILE* fp;
    if(!(fp = fopen(fpath, "r"))){
        // handle (return 404)
        char r[] = "HTTP/1.1 404 Not Found\nServer: httpc\nContent-Type: text/html\n\n\
        <h1><i>httpc</i></h1>\
        <h2>404 Not Found</h2>";
        res_body = (char*) malloc(sizeof(r));
        strncpy(res_body, r, sizeof(r));
        return res_body;
        
    }

    char* r = "HTTP/1.1 200 OK\nServer: httpc\nContent-Type: text/html\n\n";
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* str = (char*) malloc(fsize+1);
    fread(str, fsize, 1, fp);

    fclose(fp);
    str[fsize] = '\0';

    res_body = (char*) malloc(strlen(r)+strlen(str)+2);
    strcpy(res_body, r);
    strcat(res_body, str);

    free(str);
    free(fpath);

    return res_body;
}

char* process_req(char* req){
    char* method;
    char* location;

    http_head(req, &method, &location);

    printf("Method: %s\nLocation: %s\n", method, location);

    // create a response
    char* loc = location;
    if(strcmp(location, "/") == 0){
        loc = "/index.html";
    }
    char* res = get_src(loc);

    free(method);
    free(location);
    return res;
}

void http_init(unsigned short port, const char* content){

    // getting our server folder
    folder = (char*) malloc(strlen(content)+1);
    strncpy(folder, content, strlen(content)+1);

    sfd = socket(AF_INET, SOCK_STREAM, 0);

    // error while opening socket
    if(sfd == -1){
        fprintf(stderr, "(ERROR) An error occured while creating the HTTP socket!\n");
        exit(-1);
    }

    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    if (bind(sfd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        fprintf(stderr, "(ERROR) Error while binding HTTP socket!\n");
        exit(-1);
    }

    int backlog = 10;

    // error while listening
    if (listen(sfd, backlog) == -1) {
        fprintf(stderr, "(ERROR) An error has occured while trying to listen for incoming connections\n");
        exit(-1);
    }

    printf("(OK) HTTP Server is listening on port %d\n", port);
}

void http_accept() {
    int connfd;
    struct sockaddr_in cliaddr;
    unsigned int cliaddr_len = sizeof(cliaddr);

    connfd = accept(sfd, (struct sockaddr *) &cliaddr, &cliaddr_len);
    if (connfd == -1) {
        fprintf(stderr, "(ERROR) A connection error has occured");
        return;
    }

    char req[1024];
    int blen;
    // read req
    blen = read(connfd, req, sizeof(req));
    
    if(blen == -1)
    {
        // handle
        fprintf(stderr, "(ERROR) Invalid req");
        exit(-1);
    }

    printf("Request: \n%s\n",req);

    char* res = process_req(req);

    // Debug response by print
    printf("Response: \n%s\n",res);

    if(write(connfd, res, strlen(res)+1) == -1){
        // error
        fprintf(stderr, "(ERROR) An error has occured while writing to HTTP Client\n");
        return;
    }

    free(res);
    close(connfd);
}

void http_close(){
    close(sfd);
}