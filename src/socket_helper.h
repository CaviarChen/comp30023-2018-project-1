#ifndef SOCKET_HELPER_H
#define SOCKET_HELPER_H

#define READ_BUFFER_LEN 1024

int recv_line(int sockfd, char* buffer, int maxlen);
void recv_all(int sockfd);
int send_buffer(int sockfd, const char* buffer, int buffer_len);
int send_string(int sockfd, const char* str);

#endif
