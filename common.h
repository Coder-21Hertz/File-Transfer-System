#ifndef COMMON_H
#define COMMON_H

#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "tlpi_hdr.h"

#define PORT 6000
#define LISTENQ 20
#define BUFFSIZE 4096
#define FILE_NAME_MAX_SIZE 512

char my_strtok(char *buff, char filename[FILE_NAME_MAX_SIZE]);
void send_file(char *filename, SSL *ssl, char *claddrStr);
void recv_file(char *filename, SSL *ssl, char *claddrStr);
int socket_init();
void ShowCerts (SSL* ssl);


#endif // !COMMON_H