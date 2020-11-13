#ifndef __HTTPPARSER_H__
#define __HTTPPARSER_H__

#include <stdlib.h>
#include <string.h>

#define HEADER_NAME_MAX_SIZE 512
#define HEADER_VALUE_MAX_SIZE 1024

/* You won't lose style points for including this long line in your code */


typedef struct {
	char name[HEADER_NAME_MAX_SIZE];
	char value[HEADER_VALUE_MAX_SIZE];
} http_header;

int parse_request(const char *request, char *method, char *hostname, 
    char *port, char *uri, http_header *headers);

char *get_header_value(const char *name, http_header *headers, 
    int num_headers);

void generateRequest(char* method, char* hostname, char* port, char* uri, 
    http_header*headers, int numHeaders, char* req);

#endif