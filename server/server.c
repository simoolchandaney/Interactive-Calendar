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
#include "../cJSON.h"
#include <time.h>
#include <sys/stat.h>



uint16_t rec_data_sz(int new_fd) {
    //receive data size
    uint16_t data_length;
    if(recv(new_fd, &data_length, sizeof(data_length), 0) == -1) {
        perror("recv");
        exit(1);
    }

    return data_length;
}
char *rec_data(int new_fd, uint16_t data_length) {
    // receive data
    char *data = malloc(data_length+1);
    data[data_length] = '\0';
    if(recv(new_fd, data, ntohs(data_length), 0) == -1) {
        perror("recv");
        exit(1);
    }

    return data;
}

void write_back_to_file(cJSON *calendar, char *file_name) {

    FILE *fp = fopen(file_name, "w+");
    char *calendar_string = cJSON_Print(calendar);
    if(fwrite(calendar_string, 1, strlen(calendar_string), fp) == -1) {
        perror("unable to write calendar");
        exit(1);
    }
    fclose(fp);
}

char *do_add(cJSON *calendar, int new_fd, char *file_name) {

    uint16_t num_fields = rec_data_sz(new_fd);

    cJSON *entry = cJSON_CreateObject();

    for(int i = 0; i < num_fields/2; i++) {

        char *field = rec_data(new_fd, rec_data_sz(new_fd));
        char *field_value = rec_data(new_fd, rec_data_sz(new_fd));

        cJSON_AddStringToObject(entry, field, field_value);
    }

    cJSON *identifier_number = cJSON_CreateNumber((rand() % (9999999 - 1000000 + 1)) + 1000000); //create unique 3 digit identifier
    cJSON_AddItemToObject(entry, "identifier", identifier_number);
    cJSON_AddItemToArray(calendar, entry);

    write_back_to_file(calendar, file_name);
    
    return "";
}

char *do_remove(cJSON *calendar, int new_fd, char *file_name) {

    char *identifier = rec_data(new_fd, rec_data_sz(new_fd));
    int calendar_size = cJSON_GetArraySize(calendar);
    char *error = "";

    for(int i = 0; i < calendar_size; i++) {
        cJSON *entry = cJSON_GetArrayItem(calendar, i);
        if (!cJSON_GetNumberValue(cJSON_GetObjectItem(entry, "identifier"))) {
            error = "error to remove: identifier does not exist";
        }
        else {
            int curr_identifier = cJSON_GetNumberValue(cJSON_GetObjectItem(entry, "identifier"));
            if (atoi(identifier) == curr_identifier) {
                cJSON_DeleteItemFromArray(calendar, i);
            }
        }
    }

    write_back_to_file(calendar, file_name);

    return error;
}

char *do_update(cJSON *calendar, int new_fd, char *file_name) {
    char *identifier = rec_data(new_fd, rec_data_sz(new_fd));

    char *field = rec_data(new_fd, rec_data_sz(new_fd));

    //make sure identifier is not modified
    if(!strcmp(field, "identifier")) {
        perror("identifier cannot be modifed");
        exit(1);
    }

    char *field_value = rec_data(new_fd, rec_data_sz(new_fd));


    int calendar_size = cJSON_GetArraySize(calendar);
    for(int i = 0; i < calendar_size; i++) {
        cJSON *entry = cJSON_GetArrayItem(calendar, i);
        int curr_identifier = cJSON_GetNumberValue(cJSON_GetObjectItem(entry, "identifier"));
        if (atoi(identifier) == curr_identifier) {
            cJSON_ReplaceItemInObject(entry, field, cJSON_CreateString(field_value));
        }
    }

    write_back_to_file(calendar, file_name);

    return "";
}

char *do_get(cJSON *calendar, int new_fd, char *file_name) {
    //char *date = rec_data(new_fd, rec_data_sz(new_fd));
    /*
    //TODO get events as json and send to client for particular date
    int calendar_size = cJSON_GetArraySize(calendar);
    cJSON *get_events = cJSON_CreateObject();
    cJSON *events = cJSON_CreateArray();

    for(int i = 0; i < calendar_size; i++) {
        cJSON *entry = cJSON_GetArrayItem(calendar, i);
        cJSON *curr_date = cJSON_GetObjectItem(entry, "date");
        if (!strcmp(date, cJSON_Print(curr_date))) {
            cJSON *event_name = cJSON_GetObjectItem(entry, "name");
            cJSON_AddItemToArray(events, event_name);
        }
    }

    cJSON *num_events = cJSON_CreateNumber(cJSON_GetArraySize(events));
    cJSON_AddItemToObject(get_events, "numevents", num_events);
    cJSON_AddItemToObject(get_events, "data", events);
    */

    return "";
}

char *do_getrange(cJSON *calendar, int new_fd, char *file_name) {
    //char *start_date = rec_data(new_fd, rec_data_sz(new_fd));

    //char *end_date = rec_data(new_fd, rec_data_sz(new_fd));

    //TODO iterate through date range and get events

    return "";

}

char *do_input(cJSON *calendar, int new_fd, char *file_name) {
    //char *input = rec_data(new_fd, rec_data_sz(new_fd));

    //cJSON *input_list = cJSON_Parse(input);

    return "";

}

cJSON *perform_action(char *action, cJSON *calendar, int new_fd, char *file_name) {
    char *error;

    if(!strcmp(action, "add")) {
        error = do_add(calendar, new_fd, file_name);
    }

    else if(!strcmp(action, "remove")) {
        error = do_remove(calendar, new_fd, file_name);
    }

    else if(!strcmp(action, "update")) {
        error = do_update(calendar, new_fd, file_name);
    }

    else if(!strcmp(action, "get")) {
        error = do_get(calendar, new_fd, file_name);
    }

    else if(!strcmp(action, "getrange")) {
        error = do_getrange(calendar, new_fd, file_name);
    }

    else if(!strcmp(action, "input")) {
        error = do_input(calendar, new_fd, file_name);
    }

    else {
        error = "Invalid action";
    }

    //send_response_json(action, calendar, identifier, success, error, data);

    return cJSON_CreateObject();
}

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


            char *calendar_name = rec_data(new_fd, rec_data_sz(new_fd));

            //open file
            char file_name[BUFSIZ];
            strcat(file_name, "server/data/");
            strcat(file_name, calendar_name);
            strcat(file_name, ".json");
            FILE *fp = fopen(file_name, "r");

 

            //parse json file for calendar into cJSON object
            char calendar_buffer[BUFSIZ];
            if(fread(calendar_buffer, 1, BUFSIZ, fp) == -1) {
                perror("unable to read calendar");
                exit(1);
            }
            
            fclose(fp);

            cJSON *calendar = cJSON_Parse(calendar_buffer);

            char *action = rec_data(new_fd, rec_data_sz(new_fd));
            printf("action: %s\n", action);
            perform_action(action, calendar, new_fd, file_name);

            
            /*

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
            */
			fclose(fp);
            close(new_fd);
            exit(0);
        }
        close(new_fd);  // parent doesn't need this
    
    }
    return 0;
}