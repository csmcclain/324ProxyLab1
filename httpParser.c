/* $begin httpParser.c */
#include "httpParser.h"

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_conn_hdr = "Proxy-Connection: close\r\n";

int parse_request(const char *request, char *method, char *hostname, char *port, char *uri, http_header *headers) {

	char req_copy[1024];
	strcpy(req_copy, request);
	char *request_line = strtok(req_copy, "\r\n");
	char *req_ptr;
	strcpy(method, strtok_r(request_line, " ", &req_ptr));
	char url[128];
	char port_arr[128];
	strcpy(url, strtok_r(NULL," ",&req_ptr));
	strcpy(port_arr, url);
	char *url_ptr;
	strcpy(url, strtok_r(url, "//",&url_ptr));
	char hostAndPort[100];
	strcpy(hostAndPort, strtok_r(NULL, "/", &url_ptr));
	int foundPort = 0, hostInt = 0, portInt = 0;
	
    for (int i = 0; i < strlen(hostAndPort); i++) {
		if (':' == hostAndPort[i]) {
			foundPort = 1;
			continue;
		} else if (!foundPort) {
			hostname[hostInt] = hostAndPort[i];
			hostInt++;
		} else {
			port[portInt] = hostAndPort[i];
			portInt++;
		}
	}
	
	strcpy(uri, strtok_r(NULL, " ", &url_ptr));
	
	int i = 0;
	request_line = strtok(NULL, "\r\n");
	while(request_line != NULL) {
		char *name_ptr;
		strcpy(headers[i].name, strtok_r(request_line, ":", &name_ptr));
		strcpy(headers[i].value, strtok_r(NULL, " ", &name_ptr));
		request_line = strtok(NULL, "\r\n");
		i++;
	}

	return i;
}

char *get_header_value(const char *name, http_header *headers, int num_headers) {

	for (int i = 0; i < num_headers; i++) {
		if (strcmp(name, headers[i].name) == 0) {
			return headers[i].value;
		}
	}

	return NULL;
}

void generateRequest(char* method, char* hostname, char* port, char* uri, http_header*headers, int numHeaders, char* req) {

    // Generate our first line of the request
    strncat(req, method, strlen(method));
    strncat(req, " /", 2);
    strncat(req, uri, strlen(uri));
    strncat(req, " HTTP/1.0\r\n", 13);

    // Now add the rest of the headers
    int setProxy = 0, setConn = 0, setAgnet = 0;
    for (int i = 0; i < numHeaders; i++) {
        if (!strcmp(headers[i].name, "User-Agent")) {
            strncat(req, user_agent_hdr, strlen(user_agent_hdr));
            setAgnet++;
        } else if (!strcmp(headers[i].name, "Proxy-Connection")) {
            strncat(req, proxy_conn_hdr, strlen(proxy_conn_hdr));
            setProxy++;
        } else if (!strcmp(headers[i].name, "Connection")) {
            strncat(req, connection_hdr, strlen(connection_hdr));
            setConn++;
        } else {
            strncat(req, headers[i].name, strlen(headers[i].name));
            strncat(req, ": ", 2);
            strncat(req, headers[i].value, strlen(headers[i].value));
            strncat(req, "\r\n", 2);
        }
    }

    if (!setAgnet) strncat(req, user_agent_hdr, strlen(user_agent_hdr));
    if (!setProxy) strncat(req, proxy_conn_hdr, strlen(proxy_conn_hdr));
    if (!setConn) strncat(req, connection_hdr, strlen(connection_hdr));
    strncat(req, "\r\n", 2);

}