#include "mySocket.h"

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

int readClient(int cfd, char *buf) {
	ssize_t nread;
	int numRead = 0;
	do {
		nread = read(cfd, (buf + numRead), (MAX_OBJECT_SIZE - numRead));
		if (nread == -1) {
			perror("read");
			exit(EXIT_FAILURE);
		}
		numRead += nread;
		if (!strcmp(buf+(strlen(buf) - 4), "\r\n\r\n")) break;
	} while (nread != 0);

	return numRead;
}

void writeToSocket(int sfd, char* req, int numBytesRead) {
	// Write to Server Socket
    int numBytesSent = 0;
	do {
		if((numBytesSent += write(sfd, req, numBytesRead)) != numBytesRead) {
			fprintf(stderr, "partial/failed write\n");
			exit(EXIT_FAILURE);
		}
	} while (numBytesSent < numBytesRead && req[numBytesRead] != '\0');
}

int readServer(int sfd, char* res) {
	int numBytesRead = 0;
	ssize_t nread;

    do {
		nread = read(sfd, (res + numBytesRead), (MAX_OBJECT_SIZE - numBytesRead));
		if (nread == -1) {
			perror("read");
			exit(EXIT_FAILURE);
		}
		numBytesRead += nread;
		
	} while (nread != 0);

	return numBytesRead;
}
