#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "http.h"
#include "signal.h"


void on_close(int sig){
    printf("Closing server...\n");
    http_close();
    exit(0);
}

int main(int argc, char* argv[]){

    if(argc == 1){
        fprintf(stderr, "(ERROR) Please provide a folder containing server files to serve\n");
        exit(-1);
    }
    signal(SIGINT, on_close);

    http_init(6969, argv[1]);

    while (1) {
        http_accept();
    }

    return 0;
}
