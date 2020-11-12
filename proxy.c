#include <arpa/inet.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define HEADER_NAME_MAX_SIZE 512
#define HEADER_VALUE_MAX_SIZE 1024
#define MAX_HEADERS 32
#define MAX_METHOD 10
#define MAX_HOSTNAME 200
#define MAX_PORT 10
#define MAX_URI 100

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connection_hdr = "Connection: close\r\n";
static const char *proxy_conn_hdr = "Proxy-Connection: close\r\n";

typedef struct {
	char name[HEADER_NAME_MAX_SIZE];
	char value[HEADER_VALUE_MAX_SIZE];
} http_header;

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
	strcpy(uri, strtok_r(NULL, "/", &url_ptr));
	

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

int generateSocket(char *hostname, char* port) {
    struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;

    memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
	hints.ai_flags = 0;
	hints.ai_protocol = 0;          /* Any protocol */

    if (strlen(port) == 0) {
        strncat(port, "80", 2);
    }

    s = getaddrinfo(hostname, port, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

    for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype,
				rp->ai_protocol);
		if (sfd == -1)
			continue;

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;                  /* Success */

		close(sfd);
	}

    	if (rp == NULL) {               /* No address succeeded */
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result);           /* No longer needed */

    return sfd;
}


void manageClient(int cfd, struct sockaddr_storage peer_addr, socklen_t peer_addr_len) {
    ssize_t nread;
    char buf[MAX_OBJECT_SIZE];

    // Get message
    while (1) {
        peer_addr_len = sizeof(struct sockaddr_storage);
        nread = recv(cfd, buf, MAX_OBJECT_SIZE, 0);
        if (!strcmp(buf+(nread-4), "\r\n\r\n")) break;
    }
    //TODO REMOVE ME!!!
    fprintf(stderr, "%s", buf);

    // Parse message
    char method[MAX_METHOD], hostname[MAX_HOSTNAME], port[MAX_PORT], uri[MAX_URI];
    http_header headers[MAX_HEADERS];
    int numHeaders = parse_request(buf, method, hostname, port, uri, headers);

    // Construct message
    char req[MAX_OBJECT_SIZE];
    generateRequest(method, hostname, port, uri, headers, numHeaders, req);

    // Construct Server Socket
    int servfd = generateSocket(hostname, port);

    // Write to Server Socket
    int numBytesRead = strlen(req), numBytesSent = 0;
	do {
		if((numBytesSent += write(servfd, req, numBytesRead)) != numBytesRead) {
			fprintf(stderr, "partial/failed write\n");
			exit(EXIT_FAILURE);
		}
	} while (numBytesSent < numBytesRead && req[numBytesRead] != '\0');

    // Message sent Now to receive the message

    char res[MAX_OBJECT_SIZE];
    numBytesRead = 0;

    do {
		nread = read(servfd, (res + numBytesRead), (MAX_OBJECT_SIZE - numBytesRead));
		if (nread == -1) {
			perror("read");
			exit(EXIT_FAILURE);
		}
		numBytesRead += nread;
		
	} while (nread != 0);

    close(servfd);

    // Now I have the server response lets forward it on to the server!!!
    fprintf(stderr, "%s", res);

    numBytesSent = 0;
	do {
		if((numBytesSent += write(cfd, res, numBytesRead)) != numBytesRead) {
			fprintf(stderr, "partial/failed write\n");
			exit(EXIT_FAILURE);
		}
	} while (numBytesSent < numBytesRead && req[numBytesRead] != '\0');

    close(cfd);

}



int main(int argc, char *argv[])
{
    // Check to see if we have the port to listen on
    if (argc < 2) {
        printf("Usage ./proxy <port>\n");
        exit(1);
    }
    
    // Now create the socket and listen for clients to connect
	struct addrinfo hints;
	struct addrinfo *result;
    int sfd, s;
    struct sockaddr_storage peer_addr;
	socklen_t peer_addr_len;

    memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;           /* Choose IPv4 or IPv6 */
	hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
	hints.ai_flags = AI_PASSIVE;    /* Wildcard IP address - i.e., listen
					   on _all_ IPv4 or IPv6 addresses */
	hints.ai_protocol = 0;          /* Any protocol */
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;

    // Now connect the socket and listen for clients

    if ((s = getaddrinfo(NULL, argv[1], &hints, &result)) < 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

    if ((sfd = socket(result->ai_family, result->ai_socktype,
			result->ai_protocol)) < 0) {
		perror("Error creating socket");
		exit(EXIT_FAILURE);
	}

   	if (bind(sfd, result->ai_addr, result->ai_addrlen) < 0) {
		perror("Could not bind");
		exit(EXIT_FAILURE);
	}

    freeaddrinfo(result); 

    if (listen(sfd, 100) == -1) {
		fprintf(stderr, "Error listening to socket\n");
		exit(EXIT_FAILURE);
	}

    // TODO: THIS WILL HANDLE A SINGLE CLIENT AT A TIME, YOU WILL NEED TO UPDATE THIS TO HANDLE MULTIPLE CLIENTS
    int cont = 1;
    //pthread_t id[10];
    //int i = 1;
    while (cont)
    {
        int clientFD = accept(sfd, (struct sockaddr *) &peer_addr, &peer_addr_len);
        if (clientFD == -1) {
            fprintf(stderr, "Error accepting connection\n");
            exit(EXIT_FAILURE);
        }
        
        manageClient(clientFD, peer_addr, peer_addr_len);
        //cont = 0;
    }
    


    // Now pass clientFD to helper func!!!
    return 0;
}
