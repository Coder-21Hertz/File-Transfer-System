#!/bin/sh
gcc -o  client client.c -lssl -lcrypto -ldl
gcc -o  server server.c -lssl -lcrypto -ldl
