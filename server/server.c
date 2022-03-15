/*
** server.c -- a stream socket server demo
    Ian Havenaar, Simran Moolchandaney, Jacob Schwartz
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include "cJSON.h"
#include <time.h>

#define BACKLOG 10   // how many pending connections queue will hold

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    srand(time(NULL)); // for identifier

    if (argc != 2) {
        fprintf(stderr,"usage: server hostname\n");
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
        printf("server: got connection from %s\n", s);

        if (!fork()) { // this is the child process
            close(sockfd); // child doesn't need the listener


            //check for errors when client asks for calendar stuff
            char *error_message;
            
            //receive size of calendar name
            uint16_t calendar_length;
            if(recv(new_fd, &calendar_length, sizeof(calendar_length), 0) == -1) {
                perror("recv");
                exit(1);
            }

            // receive calendar name
            char calendar_name[calendar_length + 1];
            calendar_name[calendar_length] = '\0';
            if(recv(new_fd, calendar_name, ntohs(calendar_length), 0) == -1) {
                perror("recv");
                exit(1);
            }

            //open calendar and put JSON in cal_JSON
            FILE *fp = fopen(strcat(strcat("data/", calendar_name), ".json"), "w");

            if(fp == NULL) {
                //need to create JSON file
                cJSON *date = NULL;
                cJSON *time = NULL;
                cJSON *duration = NULL;
                cJSON *name = NULL;
                cJSON *description = NULL;
                cJSON *location = NULL;
                cJSON *identifier_number = cJSON_CreateNumber((rand() % (999 - 100 + 1)) + 100); //create unique 3 digit identifier

                cJSON *new_calendar = cJSON_CreateArray();
                cJSON *entry = cJSON_CreateObject();


                if(new_calendar == NULL || entry == NULL) {
                    perror("unable to write calendar.");
                    exit(1);
                }

                cJSON_AddItemToObject(entry, "date", date);
                cJSON_AddItemToObject(entry, "time", time);
                cJSON_AddItemToObject(entry, "duration", duration);
                cJSON_AddItemToObject(entry, "name", name);
                cJSON_AddItemToObject(entry, "description", description);
                cJSON_AddItemToObject(entry, "location", location);
                cJSON_AddItemToObject(entry, "indetifier_number", identifier_number);

                //add new null entry to new_calendar
                cJSON_AddItemToArray(new_calendar, entry);

                char *new_calendar_string = cJSON_Print(new_calendar);

                if(fwrite(new_calendar_string, sizeof(new_calendar_string), 1, fp) == -1) {
                    perror("unable to write calendar");
                    exit(1);
                }
            }

            //parse json file for calendar into cJSON object
            char calendar_buffer[BUFSIZ];
            if(fread(calendar_buffer, BUFSIZ, 1, fp) == -1) {
                perror("unable to read calendar");
                exit(1);
            }

            cJSON *calendar = cJSON_Parse(calendar_buffer);


            //receive size of action name
            uint16_t action_length;
            if(recv(new_fd, &action_length, sizeof(action_length), 0) == -1) {
                perror("recv");
                exit(1);
            }

            // receive action
            char action[action_length + 1];
            action[action_length] = '\0';
            if(recv(new_fd, action, ntohs(action_length), 0) == -1) {
                perror("recv");
                exit(1);
            }

            if(!strcmp(action, "add")) {

                //receive number of fields to be added
                uint16_t num_fields;
                if(recv(new_fd, &num_fields, sizeof(num_fields), 0) == -1) {
                    perror("recv");
                    exit(1);
                }

                cJSON *entry = cJSON_CreateObject();

                for(int i = 0; i < num_fields; i++) {

                    //receive size of field name
                    uint16_t field_length;
                    if(recv(new_fd, &field_length, sizeof(field_length), 0) == -1) {
                        perror("recv");
                        exit(1);
                    }

                    // receive field name
                    char field[field_length + 1];
                    field[field_length] = '\0';
                    if(recv(new_fd, field, ntohs(field_length), 0) == -1) {
                        perror("recv");
                        exit(1);
                    }

                    //receive size of field value
                    uint16_t field_value_length;
                    if(recv(new_fd, &field_value_length, sizeof(field_value_length), 0) == -1) {
                        perror("recv");
                        exit(1);
                    }

                    // receive field value
                    char field_value[field_value_length + 1];
                    field_value[field_value_length] = '\0';
                    if(recv(new_fd, field, ntohs(field_value_length), 0) == -1) {
                        perror("recv");
                        exit(1);
                    }

                    //TODO error message if field value does not exits
                    cJSON_AddStringToObject(entry, field, field_value);
                }

                cJSON_AddItemToArray(calendar, entry);

                //write json object back to file
                char *calendar_string = cJSON_Print(calendar);

                if(fwrite(calendar_string, sizeof(calendar_string), 1, fp) == -1) {
                    perror("unable to write calendar");
                    exit(1);
                }

            }
            else if(!strcmp(action, "remove")) {
               
                //receive size of identifier
                uint16_t identifier_length;
                if(recv(new_fd, &identifier_length, sizeof(identifier_length), 0) == -1) {
                    perror("recv");
                    exit(1);
                }

                // receive identifier
                char identifier[identifier_length + 1];
                identifier[identifier_length] = '\0';
                if(recv(new_fd, identifier, ntohs(identifier_length), 0) == -1) {
                    perror("recv");
                    exit(1);
                }

                //TODO PERFORM ACTION to remove event with identifier
                //TODO add error message if identifier does not exist
                int calendar_size = cJSON_GetArraySize(calendar);
                for(int i = 0; i < calendar_size; i++) {
                    cJSON *entry = cJSON_GetArrayItem(calendar, i);
                    cJSON *curr_identifier = cJSON_GetObjectItem(entry, "identifier");
                    if (!strcmp(identifier, cJSON_Print(curr_identifier))) {
                        cJSON_DeleteItemFromArray(calendar, i);
                    }
                }

                //write json object back to file
                char *calendar_string = cJSON_Print(calendar);

                if(fwrite(calendar_string, sizeof(calendar_string), 1, fp) == -1) {
                    perror("unable to write calendar");
                    exit(1);
                }

            }
            else if(!strcmp(action, "update")) {
                //receive size of identifier
                uint16_t identifier_length;
                if(recv(new_fd, &identifier_length, sizeof(identifier_length), 0) == -1) {
                    perror("recv");
                    exit(1);
                }

                // receive identifier
                char identifier[identifier_length + 1];
                identifier[identifier_length] = '\0';
                if(recv(new_fd, identifier, ntohs(identifier_length), 0) == -1) {
                    perror("recv");
                    exit(1);
                }

                //receive size of field name
                uint16_t field_length;
                if(recv(new_fd, &field_length, sizeof(field_length), 0) == -1) {
                    perror("recv");
                    exit(1);
                }

                // receive field name
                char field[field_length + 1];
                field[field_length] = '\0';
                if(recv(new_fd, field, ntohs(field_length), 0) == -1) {
                    perror("recv");
                    exit(1);
                }

                //make sure identifier is not modified
                if(!strcmp(field, "identifier")) {
                    perror("identifier cannot be modifed");
                    exit(1);
                }

                //receive size of field value
                uint16_t field_value_length;
                if(recv(new_fd, &field_value_length, sizeof(field_value_length), 0) == -1) {
                    perror("recv");
                    exit(1);
                }

                // receive field value
                char field_value[field_value_length + 1];
                field_value[field_value_length] = '\0';
                if(recv(new_fd, field, ntohs(field_value_length), 0) == -1) {
                    perror("recv");
                    exit(1);
                }


                //TODO update field with field value based on identifier
                int calendar_size = cJSON_GetArraySize(calendar);
                for(int i = 0; i < calendar_size; i++) {
                    cJSON *entry = cJSON_GetArrayItem(calendar, i);
                    cJSON *curr_identifier = cJSON_GetObjectItem(entry, "identifier");
                    if (!strcmp(identifier, cJSON_Print(curr_identifier))) {
                        cJSON_ReplaceItemInObject(entry, field, cJSON_CreateString(field_value));
                    }
                }

                //write json object back to file
                char *calendar_string = cJSON_Print(calendar);

                if(fwrite(calendar_string, sizeof(calendar_string), 1, fp) == -1) {
                    perror("unable to write calendar");
                    exit(1);
                }

            }
            else if(!strcmp(action, "get")) {
                //receive size of date value
                uint16_t date_length;
                if(recv(new_fd, &date_length, sizeof(date_length), 0) == -1) {
                    perror("recv");
                    exit(1);
                }

                // receive date value
                char date[date_length + 1];
                date[date_length] = '\0';
                if(recv(new_fd, date, ntohs(date_length), 0) == -1) {
                    perror("recv");
                    exit(1);
                }

                //TODO get events as json and send to client for particular date

            }
            else if(!strcmp(action, "getrange")) {
                //receive size of start date value
                uint16_t start_date_length;
                if(recv(new_fd, &start_date_length, sizeof(start_date_length), 0) == -1) {
                    perror("recv");
                    exit(1);
                }

                //receive start date value
                char start_date[start_date_length + 1];
                start_date[start_date_length] = '\0';
                if(recv(new_fd, start_date, ntohs(start_date_length), 0) == -1) {
                    perror("recv");
                    exit(1);
                }

                //receive size of end date value
                uint16_t end_date_length;
                if(recv(new_fd, &end_date_length, sizeof(end_date_length), 0) == -1) {
                    perror("recv");
                    exit(1);
                }

                // receive end date value
                char end_date[end_date_length + 1];
                end_date[end_date_length] = '\0';
                if(recv(new_fd, end_date, ntohs(end_date_length), 0) == -1) {
                    perror("recv");
                    exit(1);
                }

                //TODO iterate through date range and get events

            }/*
            else if(!strcmp(action, "input")) {

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


                // receive file data and write to json
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

                    if(n > 0){
                        //TODO write file data to calendar
                        //if(write(fd, buffer, n) == -1) {
                        //    perror("write");
                        //    exit(1);
                        //}
                        break;
                    }
                
                    if(counter >= numbytes) {
                        break;
                    }
            }*/
            else {
                perror("not a valid action");
                exit(1);
            }

            fclose(fp);

            //create response json
            cJSON *response = cJSON_CreateObject();
            if(response == NULL) {
                perror("JSON response could not be created");
                exit(1);
            }

            cJSON_AddItemToObject(response, "command", cJSON_CreateString(action));
            cJSON_AddItemToObject(response, "calendar", cJSON_CreateString(calendar_name));
            //cJSON_AddItemToObject(response, "identifier", success ? identifier_number: "XXXX");
            //cJSON_AddItemToObject(response, "success", success ? "True": "False");
            //cJSON_AddItemToObject(response, "error", error_message); //TODO get error messages
            //cJSON_AddItemToObject(response, "data", data);

            //TODO send response json object to client

            uint16_t response_size = htons(sizeof(response));//miracle if this is right

            //send size of JSON
            if((send(sockfd, &response_size, sizeof(response_size), 0)) == -1) {
                perror("recv");
                exit(1);
            }

            //send JSON, no way this actually works
            if((send(sockfd, response, sizeof(response), 0)) == -1) {
                perror("recv");
                exit(1);
            }

			fclose(fp);
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    
    }
    return 0;
}