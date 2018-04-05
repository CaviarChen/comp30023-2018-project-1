#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define BACKLOG 10 // max connections in the queue

#define METHOD_GET "GET"
#define DEFAULT_PAGE "index.html"

#define READ_BUFFER_LEN 1024
#define URL_MAX_LEN 512+1


// TO-DO:
// maximum url(method) len? is buffer enough?




void print_prompt();
int recv_line(int sockfd, char* buffer, int maxlen);

int main(int argc, char const *argv[]) {

    int port_no = 0;
    char root_path[PATH_MAX];

    if (argc < 3) {
        print_prompt();
    } else {
        port_no = atoi(argv[1]);
        realpath(argv[2], root_path);
    }

    // Create TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("ERROR opening socket");
        exit(EXIT_FAILURE);
    }

    // set ip and port for binding
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_no);

    // bind
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        perror("ERROR on binding");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("ERROR listen");
        exit(EXIT_FAILURE);
    }

    printf("Serving HTTP on 0.0.0.0 port %d ...\n", port_no);

    struct sockaddr_in cli_addr;
    socklen_t cli_addr_len = sizeof(cli_addr);

    // main loop
    while (1) {
        int new_sockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
                                                            &cli_addr_len);

        if (new_sockfd < 0) {
            perror("ERROR on accept");
            continue;
        }

        // read and parse http header
        char buffer[READ_BUFFER_LEN];
        int buffer_len;
        buffer_len = recv_line(new_sockfd, buffer, READ_BUFFER_LEN);


        // while (buffer_len>0 && buffer[0]!='\n') {
        //     printf("%d\n", buffer_len);
        //     printf("%s\n", buffer);
        //     buffer_len = recv_line(new_sockfd, buffer, READ_BUFFER_LEN);
        // }

        // parse method
        int method_len;
        for(method_len = 0;(method_len<buffer_len)&&(buffer[method_len]!=' ');
                                                                 method_len++);
        if (strncmp(buffer, METHOD_GET, method_len) != 0) {
            // not implement
            perror("ERROR not implement");
        }

        // parse url
        char url[URL_MAX_LEN];
        int url_len;
        for(url_len=0; (method_len+url_len+1)<buffer_len; url_len++) {
            char c = buffer[method_len+url_len+1];
            // stop when meet ' ' or '?'
            // since only server static file, ignore things after ?
            if (c==' ' || c=='?') break;
            url[url_len] = c;
        }
        url[url_len] = '\0';


        // get file path
        // root_path+url can be unsafe since there can be /../../
        char filepath_unsafe[PATH_MAX];
        snprintf(filepath_unsafe, sizeof(filepath_unsafe), "%s%s",
                                            root_path, url);
        if (url[url_len-1]=='/') {
            // add index.html when the url is a directory
            snprintf(filepath_unsafe, sizeof(filepath_unsafe), "%s%s%s",
                                                root_path, url, DEFAULT_PAGE);
        } else {
            snprintf(filepath_unsafe, sizeof(filepath_unsafe), "%s%s",
                                                root_path, url);
        }

        char filepath[PATH_MAX];
        realpath(filepath_unsafe, filepath);

        if(strncmp(filepath, root_path, strlen(root_path))!=0) {
            // file is outside root_path
            perror("ERROR 403");
        }



        // char buf[1024];
        //
        // sprintf(buf, "HTTP/1.0 404 NOT FOUND\r\n");
        // send(new_sockfd, buf, strlen(buf), 0);

        close(new_sockfd);

    }



    return 0;
}

int recv_line(int sockfd, char* buffer, int maxlen) {
    int i = 0;
    while(i < (maxlen-1)) {
        char c;
        // read one char
        int n = recv(sockfd, &c, 1, 0);
        if (n<=0) {
            return n;
        }

        // skip '\r'
        if (c == '\r') continue;

        buffer[i] = c;
        i++;

        // stop
        if (c == '\n') break;
    }

    buffer[i] = '\0';
    return i;
}


void print_prompt() {
    printf("Usage:\t\tserver [port number] [path to web root]\n");

    exit(EXIT_FAILURE);

}
