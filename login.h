#ifndef LOGIN_H
#define LOGIN_H

#include <mysql/mysql.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "tlpi_hdr.h"

char passwd_d[20];

int callback(void *NotUsed, int argc, char *argv[], char **azColName);
char server_login(SSL *ssl);
int client_login(SSL *ssl);

#endif // !LOGIN_H
