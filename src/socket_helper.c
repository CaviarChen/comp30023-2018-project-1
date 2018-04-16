/* Simple HTTP1.0 Server (Socket Helper)
 * Name: Zijun Chen
 * StudentID: 813190
 * LoginID: zijunc3
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "socket_helper.h"

// send a string
int send_string(int sockfd, const char* str) {
    return send_buffer(sockfd, str, strlen(str));
}

// send a buffer
int send_buffer(int sockfd, const char* buffer, int buffer_len) {
    const char* p = buffer;
    // keep sending
    while (buffer_len>0) {
        int n = send(sockfd, p, buffer_len, 0);
        if (n<1) return n;
        buffer_len -= n;
        p += n;
    }
    return 1;
}

// read a line of data from request
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

// read and ignore all data from request
void recv_all(int sockfd) {
    char buffer[READ_BUFFER_LEN];
    int buffer_len;

    // keep reading until meet a empty line
    do {
        buffer_len = recv_line(sockfd, buffer, READ_BUFFER_LEN);
    } while(buffer_len>0 && buffer[0]!='\n');
}
