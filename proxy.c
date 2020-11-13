#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include "sbuf.h"
#include "httpParser.h"
#include "mySocket.h"



/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

#define MAX_HEADERS 32
#define MAX_METHOD 10
#define MAX_HOSTNAME 200
#define MAX_PORT 10
#define MAX_URI 100

// These are needed for threads
#define NTHREADS  20
#define SBUFSIZE  20

void manageClient(int cfd) {
    char *buf = calloc(MAX_OBJECT_SIZE, sizeof(char));
	// Get message
	int numBytesRead = readClient(cfd, buf);


    // Parse message
    char *method = calloc(MAX_METHOD, sizeof(char));
	char *hostname =calloc(MAX_HOSTNAME, sizeof(char));
	char *port = calloc(MAX_PORT, sizeof(char));
	char *uri = calloc(MAX_URI, sizeof(char));;
    http_header headers[MAX_HEADERS];
    int numHeaders = parse_request(buf, method, hostname, port, uri, headers);
	free(buf);

    // Construct message
    char *req = calloc(MAX_OBJECT_SIZE, sizeof(char));
    generateRequest(method, hostname, port, uri, headers, numHeaders, req);

    // Construct Server Socket
    int servfd = generateSocket(hostname, port);
	free(method);
	free(hostname);
	free(port);
	free(uri);

	// Write to server
	writeToSocket(servfd, req, strlen(req));
	free(req);

    // Message sent Now to receive the message
    char *res = calloc(MAX_OBJECT_SIZE, sizeof(char));
	numBytesRead = readServer(servfd, res);
    close(servfd);

    // Now I have the server response lets forward it on to the server!!!
	writeToSocket(cfd, res, numBytesRead);

    close(cfd);
	free(res);

}

// thread stuff goes here!!!
sbuf_t sbuf;
pthread_t tid[NTHREADS];

void *thread(void *vargp) {
	free(vargp);
	while (1) {
		int cfd = sbuf_remove(&sbuf);
		manageClient(cfd);
	}
}

void cleanup() {
	sbuf_deinit(&sbuf);
	for (int i = 0; i < NTHREADS; i++) {
		if(pthread_cancel(tid[i]) != 0) {
			fprintf(stderr, "thread cancel failed\n");
		}	
	}

	for (int i = 0; i < NTHREADS; i++) {
		if(pthread_join(tid[i], NULL) != 0) {
			fprintf(stderr, "thread cancel failed\n");
		}	
	
	}

	exit(EXIT_SUCCESS);
}

void sigHandler(int sig) {
	cleanup();
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
	

	sbuf_init(&sbuf, SBUFSIZE); //line:conc:pre:initsbuf
	signal(SIGINT, sigHandler);
	signal(SIGTERM, sigHandler);
	for (int i = 0; i < NTHREADS; i++)  /* Create worker threads */ //line:conc:pre:begincreate
		pthread_create(&tid[i], NULL, thread, NULL);               //line:conc:pre:endcreate

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


	int clientFD;
    while (1) {
     	clientFD = accept(sfd, (struct sockaddr *) &peer_addr, &peer_addr_len);
		sbuf_insert(&sbuf, clientFD);
    }

    return 0;
}
