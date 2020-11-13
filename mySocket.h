#ifndef __MYSOCKET_H__
#define __MYSOCKET_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_OBJECT_SIZE 102400

int generateSocket(char *hostname, char* port);
int readClient(int cfd, char *buf);
void writeToSocket(int sfd, char* req, int numBytesRead);
int readServer(int sfd, char* res);

#endif