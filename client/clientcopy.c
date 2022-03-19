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
#include "../cJSON.h"

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void do_add(int sockfd, int argc, char *argv[]) {
    // count num of fields to be added
    uint16_t num_fields = 0;
    for(int i = 3; i < argc; i++) {
        num_fields++;
    }
    // send num_fields
    if ((send(sockfd, &num_fields, sizeof(num_fields), 0)) == -1) {
        perror("recv");
        exit(1);  
    }
    for(int j = 3; j < argc; j++) {
        char *field = argv[j];
        uint16_t field_sz = htons(strlen(field));
        // send size of field value
        if ((send(sockfd, &field_sz, sizeof(field_sz), 0)) == -1) {
            perror("recv");
            exit(1);  
        }
        // send field value
        if ((send(sockfd, field, strlen(field), 0)) == -1) {
            perror("recv");
            exit(1);  
        }
    }
}

void do_remove(int sockfd, char *argv[]) {
    char *identifier = argv[3];
    uint16_t identifier_sz = htons(strlen(identifier));
    // send size of identifier
    if ((send(sockfd, &identifier_sz, sizeof(identifier_sz), 0)) == -1) {
            perror("recv");
            exit(1);  
    }
    // send identifier
    if ((send(sockfd, identifier, strlen(identifier), 0)) == -1) {
            perror("recv");
            exit(1);  
    }
}

void do_update(int sockfd, int argc, char *argv[]) {
    char *identifier = argv[3];
    uint16_t identifier_sz = htons(strlen(identifier));
    // send size of identifier
    if ((send(sockfd, &identifier_sz, sizeof(identifier_sz), 0)) == -1) {
            perror("recv");
            exit(1);  
    }
    // send identifier
    if ((send(sockfd, identifier, strlen(identifier), 0)) == -1) {
        perror("recv");
        exit(1);  
    }
    for(int i = 4; i < argc; i++) {
        char *field = argv[i];
        uint16_t field_sz = htons(strlen(field));
        if ((send(sockfd, &field_sz, sizeof(field_sz), 0)) == -1) {
            perror("recv");
            exit(1);  
        }
        if ((send(sockfd, field, strlen(field), 0)) == -1) {
            perror("recv");
            exit(1);  
        }
    }
}

void do_get(int sockfd, char *argv[]) {
    char *date = argv[3];
    uint16_t date_sz = htons(strlen(date));
    if ((send(sockfd, &date_sz, sizeof(date_sz), 0)) == -1) {
        perror("recv");
        exit(1);  
    }
    if ((send(sockfd, date, strlen(date), 0)) == -1) {
        perror("recv");
        exit(1);  
    }
}

void do_get_range(int sockfd, char *argv[]) {
    char *start_date = argv[3];
    uint16_t start_date_sz = htons(strlen(start_date));
    // send start_data size
    if ((send(sockfd, &start_date_sz, sizeof(start_date_sz), 0)) == -1) {
        perror("recv");
        exit(1);  
    }
    // send start_date
    if ((send(sockfd, start_date, strlen(start_date), 0)) == -1) {
        perror("recv");
        exit(1);  
    } 
    char *end_date = argv[4];
    uint16_t end_date_sz = htons(strlen(end_date));
    // send end_date size
    if ((send(sockfd, &end_date_sz, sizeof(end_date_sz), 0)) == -1) {
        perror("recv");
        exit(1);  
    }
    // send end_date
    if ((send(sockfd, end_date, strlen(end_date), 0)) == -1) {
        perror("recv");
        exit(1);  
    }
}

int main(int argc, char *argv[])
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
	// struct sockaddr_in *h;
    int rv;
    char s[INET6_ADDRSTRLEN];
	char *ip = "129.74.152.141";
    char *port = "41111";

    if (argc < 4) {
        fprintf(stderr,"usage: CalendarName action -> data for action <-\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;


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
  
    char *calendar_name = argv[1];
    uint16_t calenadr_name_sz = htons(strlen(calendar_name));

    // send size of calendar name
    if ((send(sockfd, &calenadr_name_sz, sizeof(calenadr_name_sz), 0)) == -1) {
        perror("recv");
        exit(1);  
    }

    // send calendar name to server 
    if ((send(sockfd, calendar_name, strlen(calendar_name), 0)) == -1) {
        perror("recv");
        exit(1);  
    }

    char *action_name = argv[2];
    uint16_t action_name_sz = htons(strlen(action_name));

    // send size of action
    if ((send(sockfd, &action_name_sz, sizeof(action_name_sz), 0)) == -1) {
        perror("recv");
        exit(1);  
    }

    // send action 
    if ((send(sockfd, action_name, strlen(action_name), 0)) == -1) {
        perror("recv");
        exit(1);  
    }

    if(!strcmp(action_name, "add")) {
        do_add(sockfd, argc, argv);
    }
    else if(!strcmp(action_name, "remove")) {
        do_remove(sockfd, argv);
    }
    else if(!strcmp(action_name, "update")) {
        do_update(sockfd, argc, argv);
    }
    else if(!strcmp(action_name, "get")) {
        do_get(sockfd, argv);
    }
    else if(!strcmp(action_name, "getrange")) {
        do_get_range(sockfd, argv);
    }

    else if(!strcmp(action_name, "input")) {
        char *input_file_name = argv[3];
        uint16_t input_file_name_sz = htons(strlen(input_file_name));
        FILE *fp = fopen(input_file_name, "r");
        //parse json file for calendar into cJSON object
        char calendar_buffer[BUFSIZ];
        if(fread(calendar_buffer, 1, BUFSIZ, fp) == -1) {
            perror("unable to read calendar");
            exit(1);
        }
        fclose(fp);
        cJSON *calendar = cJSON_Parse(calendar_buffer);
        int sz = cJSON_GetArraySize(calendar);
        for (int i = 0; i < sz; i++) {
            char *str = cJSON_GetStringValue(cJSON_GetArrayItem(calendar, i));
            char delim[] = " ";
            char *ptr = strtok(str, delim);
            char arr[BUFSIZ];
            int i = 0;
	        while(ptr != NULL) {
                arr[i] = ptr;
                i++;
		        ptr = strtok(NULL, delim);
	        }
            if(!strcmp(arr[2], "add")) {
                do_add(sockfd, i, arr);
            }
            else if(!strcmp(arr[2], "remove")) {
                do_remove(sockfd, arr);
            }
            else if(!strcmp(arr[2], "update")) {
                do_update(sockfd, i, arr);
            }
            else if(!strcmp(arr[2], "get")) {
                do_get(sockfd, arr);
            }
            else if(!strcmp(arr[2], "getrange")) {
                do_get_range(sockfd, arr);
            }
        }
    }

    else {
        printf("Invalid command. Please try again.\n");
        exit(1);
    }

    // receive size of json file
    uint32_t numbytes;
    if ((recv(sockfd, &numbytes, sizeof(numbytes), 0)) == -1) {
			perror("recv");
			exit(1);
	}
    numbytes = ntohl(numbytes);

	int fd = open("response.json", O_CREAT|O_RDWR, 0666);

    if (fd == -1) {
        perror("unable to open file");
    }

    // receive json file data and write to json file
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
    return 0;
}