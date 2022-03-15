/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <limits.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// used to calculate time intervals 
double timestamp() {
    struct timeval current_time;
    if (gettimeofday(&current_time, NULL) < 0) {
        return time(NULL);
    }
    return (double) current_time.tv_sec + ((double) current_time.tv_usec / 1000000.0);
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
	struct sockaddr_in *h;
    int rv;
    char s[INET6_ADDRSTRLEN];
	char *ip = "129.74.152.73";
    char *port = "41999";

    if (argc != 4) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    // argv[1] - IP
    // argv[2] - port number
    if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {

        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(servinfo); // all done with this structure

    /*
    //EC2 filter IPs
	char temp1[100];
	char temp2[100];
	char temp3[100];

	strncpy(temp1, &ip[0], 7);
	strncpy(temp2, &ip[0], 4);
	strncpy(temp3, &ip[0], 8);

	if(!(strcmp(temp1, "129.74.") == 0 || strcmp(temp2, "127.") == 0 || strcmp(temp3, "192.168.") == 0)) {
		perror("connection from invalid IP");
		exit(1);
	}
    */
  
    char *file_name = argv[3];
    uint16_t file_name_sz = htons(strlen(file_name));
	double t_init_f = timestamp(); // start timer

    // send size of file name
    if ((send(sockfd, &file_name_sz, sizeof(file_name_sz), 0)) == -1) {
        perror("recv");
        exit(1);  
    }

    // send file name to server 
    if ((send(sockfd, file_name, strlen(file_name), 0)) == -1) {
        perror("recv");
        exit(1);  
    }

    // receive size of file
    uint32_t numbytes;
    if ((recv(sockfd, &numbytes, sizeof(numbytes), 0)) == -1) {
			perror("recv");
			exit(1);
	}
    numbytes = ntohl(numbytes);

	int fd = open(argv[3], O_CREAT|O_RDWR, 0666);

    if (fd == -1) {
        perror("unable to open file");
    }


    // receive file data and write to file
    char buffer[BUFSIZ];
    int counter = 0;
    int n;
    while(1) {

	    bzero(buffer, sizeof(buffer));

        n = recv(sockfd, buffer, sizeof(buffer), 0);
		if(n == -1) {
            perror("recv");
            exit(1);
        }
		counter += n;

		if(n > 0)
			if(write(fd, buffer, n) == -1) {
                perror("write");
                exit(1);
            }
	
		if(counter >= numbytes) {
			break;
    	}
	}
	close(sockfd);
	close(fd);

    // end of transmission time
	double t_final_f = timestamp();

    // statistics calculations and reporting 
	double time_elapsed = t_final_f - t_init_f;
	double speed = (numbytes * (0.000001)) / (time_elapsed) ;
	printf("%d bytes transferred over %lf seconds for a speed of %lf MB/s\n", numbytes, time_elapsed, speed);

    return 0;
}