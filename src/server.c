#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "debug_setting.h"
#include "thread_pool.h"

#define BACKLOG 10 // max connections in the queue
#define THREAD_NUM 10

#define METHOD_GET "GET"
#define DEFAULT_PAGE "index.html"

#define READ_BUFFER_LEN 1024
#define URL_MAX_LEN 512+1
#define WRITE_BUFFER_LEN 1024
#define FILE_BUFFER_LEN 1024

typedef struct {
    int sockfd;
    const char* root_path;
} thread_arg_t;


void print_prompt();

int recv_line(int sockfd, char* buffer, int maxlen);
void recv_all(int sockfd);
int send_buffer(int sockfd, const char* buffer, int buffer_len);
int send_string(int sockfd, const char* str);

int serve_static_file(int sockfd, const char* filepath, int worker_id);
int parse_request(int sockfd, char* filepath, const char* root_path,
                                                        int worker_id);

int response_header(int sockfd, const char* filepath, int code, int worker_id);
const char* get_mime_by_exten(const char* extension);

const char* get_file_extension(const char* filepath);

void thread_fun(int worker_id, void* arg);

int main(int argc, char const *argv[]) {

    int port_no = 0;
    char root_path[PATH_MAX];

    if (argc < 3) {
        print_prompt();
    } else {
        port_no = atoi(argv[1]);
        (void) realpath(argv[2], root_path);
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

    // prevent problem caused by SIGPIPE signal
    signal(SIGPIPE, SIG_IGN);

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

    // thread_pool
    void* tp = thread_pool_init(THREAD_NUM);

    // main loop
    while (1) {
        int new_sockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
                                                            &cli_addr_len);

        if (new_sockfd < 0) {
            perror("ERROR on accept");
            continue;
        }

        thread_arg_t *arg = malloc(sizeof(thread_arg_t));
        arg->sockfd = new_sockfd;
        arg->root_path = root_path;

        thread_pool_add_task(tp, &thread_fun, arg);
    }

    return 0;
}

int response_header(int sockfd, const char* filepath, int code,
                                                    int worker_id) {
    const char* mime = NULL;
    const char* code_desc = NULL;
    switch (code) {
        case 200:
            code_desc = "200 OK";
            mime = get_mime_by_exten(get_file_extension(filepath));
            break;
        case 404:
            code_desc = "404 Not Found";
            mime = get_mime_by_exten("html");
            break;
        case 405:
            code_desc = "405 Method Not Allowed";
            mime = get_mime_by_exten("html");
            break;
    }

    #if DEBUG
        printf("(worker:%d) [%s] %s\n",worker_id, filepath, code_desc);
    #endif

    char buffer[WRITE_BUFFER_LEN];
    snprintf(buffer, WRITE_BUFFER_LEN,
        "HTTP/1.0 %s\r\nContent-Type: %s\r\n\r\n",
        code_desc, mime);
    if (send_string(sockfd, buffer)!=1) return -1;

    // if the code is not 200, then send error page
    if (code != 200) {
        char buffer[WRITE_BUFFER_LEN];
        snprintf(buffer, WRITE_BUFFER_LEN,
            "<!DOCTYPE html><html><head><meta charset='UTF-8'>\
            <title>Error</title></head><body><h1>%s</h1></body></html>",
            code_desc);
        if (send_string(sockfd, buffer)!=1) return -1;
    }
    return 0;
}

int serve_static_file(int sockfd, const char* filepath, int worker_id) {
    // R_OK for read-only
    if( access(filepath, R_OK ) == -1 ) {
        // file not exists
        response_header(sockfd, filepath, 404, worker_id);
        return -1;
    }

    response_header(sockfd, filepath, 200, worker_id);

    int n = 0;
    char buffer[FILE_BUFFER_LEN];

    FILE* file = fopen(filepath, "r");
    if (file) {
        while ((n = fread(buffer, sizeof(*buffer), sizeof(buffer), file)) > 0) {
            if (send_buffer(sockfd, buffer, n)<1) {
                fclose(file);
                return -1;
            }
        }

        if (ferror(file)) return -1;
        fclose(file);
        return 1;
    }

    return -1;
}

int send_string(int sockfd, const char* str) {
    return send_buffer(sockfd, str, strlen(str));
}

int send_buffer(int sockfd, const char* buffer, int buffer_len) {
    const char* p = buffer;
    while (buffer_len>0) {
        int n = send(sockfd, p, buffer_len, 0);
        if (n<1) return n;
        buffer_len -= n;
        p += n;
    }
    return 1;
}

int recv_line(int sockfd, char* buffer, int maxlen) {
    int i = 0;
    while(i < (maxlen-1)) {
        char c;
        // read one char
        int n = recv(sockfd, &c, 1, 0);
        if (n<=0) return n;

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

void recv_all(int sockfd) {
    char buffer[READ_BUFFER_LEN];
    int buffer_len;

    do {
        buffer_len = recv_line(sockfd, buffer, READ_BUFFER_LEN);
    } while(buffer_len>0 && buffer[0]!='\n');
}

int parse_request(int sockfd, char* filepath, const char* root_path
                                                    , int worker_id) {
    // read and parse http header
    char buffer[READ_BUFFER_LEN];
    int buffer_len;
    buffer_len = recv_line(sockfd, buffer, READ_BUFFER_LEN);

    // discard other parts of header
    recv_all(sockfd);

    // parse method
    int method_len;
    for(method_len = 0;(method_len<buffer_len)&&(buffer[method_len]!=' ');
                                                             method_len++);
    if (strncmp(buffer, METHOD_GET, method_len) != 0) {
        // method not allowed
        response_header(sockfd, filepath, 405, worker_id);
        return -1;
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

    #if DEBUG
        printf("(worker:%d) [%s] \n", worker_id, url);
    #endif

    // get file path
    // root_path+url can be unsafe since there can be /../../ (Directory
    // traversal attack)
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

    (void) realpath(filepath_unsafe, filepath);

    if ((strncmp(filepath, root_path, strlen(root_path))!=0)||
        (strncmp(filepath, filepath_unsafe, strlen(filepath))!=0)) {
        // there is something wrong with the path
        // Directory traversal attack or path not exists
        response_header(sockfd, filepath_unsafe, 404, worker_id);
        return -1;
    }
    return 1;
}

const char* get_mime_by_exten(const char* extension) {
    if (extension) {
        // case insensitive
        if (strcasecmp(extension, "html") == 0)
            return "text/html";
        if (strcasecmp(extension, "jpg") == 0)
            return "image/jpeg";
        if (strcasecmp(extension, "css") == 0)
            return "text/css";
        if (strcasecmp(extension, "js") == 0)
            return "application/javascript";
    }

    // for unknown types
    return "application/octet-stream";
}

const char* get_file_extension(const char* filepath) {
    const char* p = strrchr(filepath, '.');
    return (p)? p+1 : NULL;
}

void thread_fun(int worker_id, void* arg) {

    // get arguments
    int new_sockfd = ((thread_arg_t *)arg)->sockfd;
    const char * root_path = ((thread_arg_t *)arg)->root_path;

    free(arg);
    arg = NULL;

    char filepath[PATH_MAX];
    if(parse_request(new_sockfd, filepath, root_path, worker_id)>0) {
        int code = serve_static_file(new_sockfd, filepath, worker_id);

        #if DEBUG
            printf("(worker:%d) [%s] finished [code:%d]\n",worker_id,
                                                        filepath, code);
        #endif
    }
    close(new_sockfd);
}

void print_prompt() {
    printf("Usage:\t\tserver [port number] [path to web root]\n");

    exit(EXIT_FAILURE);

}
