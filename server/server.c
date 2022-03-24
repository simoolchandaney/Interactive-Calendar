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
    data_length = ntohs(data_length);
    char *data = malloc(data_length+1);
    data[data_length] = '\0';
    if(recv(new_fd, data, data_length, 0) == -1) {
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

int date_to_int(char *date) {
    char year[2];
    memcpy(year, &date[4], 2);
    year[2] = '\0';
    char month_day[4];
    memcpy(month_day, &date[0], 4);
    month_day[4] = '\0';
    char *date_reformat = malloc(6);
    strcat(date_reformat, year);
    strcat(date_reformat, month_day);
    int date_int = atoi(date_reformat);
    free(date_reformat);
    return date_int;
}

void send_response_json(int new_fd, char *action, char *calendar, int identifier, int success, char *error, cJSON *data) {
    //create response json
    cJSON *response = cJSON_CreateObject();
    cJSON_AddItemToObject(response, "command", cJSON_CreateString(action));
    cJSON_AddItemToObject(response, "calendar", cJSON_CreateString(calendar));
    cJSON_AddItemToObject(response, "identifier", success ? cJSON_CreateNumber(identifier): cJSON_CreateString("XXXX"));
    cJSON_AddItemToObject(response, "success",  success ? cJSON_CreateString("True"): cJSON_CreateString("False"));
    cJSON_AddItemToObject(response, "error", cJSON_CreateString(error)); 
    cJSON_AddItemToObject(response, "data", data);

    char *response_str = cJSON_Print(response);
    uint32_t response_size = htons(strlen(response_str));

    // send size of JSON
    if((send(new_fd, &response_size, sizeof(response_size), 0)) == -1) {
        perror("recv");
        exit(1);
    }
    if((send(new_fd, response_str, ntohs(response_size), 0)) == -1) {
        perror("recv");
        exit(1);
    }  
}

void do_add(cJSON *calendar, int new_fd, char *file_name, char *action, char *calendar_name) {

    char fields[6][20]= {"date", "time", "duration", "name", "description", "location"};
    char req_fields[4][20] = {"date", "time", "duration", "name"};
    int field_check[4] = {0,0,0,0};
    char *error = "";
    uint16_t num_fields = rec_data_sz(new_fd);
    cJSON *entry = cJSON_CreateObject();

    for(int i = 0; i < num_fields/2; i++) {
        char *field = rec_data(new_fd, rec_data_sz(new_fd));
        // mark required fields
        for(int i = 0; i < sizeof(req_fields)/sizeof(req_fields[0]); i++) {
            if(!strcmp(req_fields[i], field)) {
                field_check[i] = 1;
            }
        }
        int valid_field = 0;
        for(int i = 0; i < sizeof(fields)/sizeof(fields[0]); i++) {
            if(!strcmp(fields[i], field)) {
                valid_field = 1;
            }
        }  
        char *field_value = rec_data(new_fd, rec_data_sz(new_fd));
        cJSON_AddStringToObject(entry, field, field_value);
        if(!valid_field) {
            error =  "ADD Error: invalid field value";
        }  
        free(field);
        free(field_value);
    }
    for(int i = 0; i < sizeof(field_check)/ sizeof(field_check[0]); i++) {
        if(field_check[i] == 0) {
            error = "ADD Error: date, time, duration, name fields are required.";
        }
    }
    
    int is_unique = 0;
    cJSON* identifier_number = cJSON_CreateNumber(0);

    while(is_unique == 0 && cJSON_GetNumberValue(identifier_number) == 0) {
        identifier_number = cJSON_CreateNumber((rand() % (9999999 - 1000000 + 1)) + 1000000);
        cJSON *entry;
        is_unique = 1;
        cJSON_ArrayForEach(entry, calendar) {
            if(cJSON_GetNumberValue(cJSON_GetObjectItem(entry, "indentifier")) == cJSON_GetNumberValue(identifier_number)) {
                is_unique = 0;
            }
        }
    }
    
    cJSON_AddItemToObject(entry, "identifier", identifier_number);
    cJSON_AddItemToArray(calendar, entry);

    if(!strcmp(error, "")) {
        write_back_to_file(calendar, file_name);
    }
    int success = !strcmp(error, "");
    send_response_json(new_fd, action, calendar_name, cJSON_GetNumberValue(cJSON_GetObjectItem(entry, "identifier")), success, error, cJSON_CreateObject());
    return;
}

void do_remove(cJSON *calendar, int new_fd, char *file_name, char *action, char *calendar_name) {
    char *identifier = rec_data(new_fd, rec_data_sz(new_fd));
    int calendar_size = cJSON_GetArraySize(calendar);
    char *error = "";
    cJSON *entry = NULL;
    int found_identifier = 0;
    int identifier_value = NULL;
    for(int i = 0; i < calendar_size; i++) {
        entry = cJSON_GetArrayItem(calendar, i);
        int curr_identifier = cJSON_GetNumberValue(cJSON_GetObjectItem(entry, "identifier"));
        if (atoi(identifier) == curr_identifier) {
            found_identifier = 1;
            identifier_value = curr_identifier;
            cJSON_DeleteItemFromArray(calendar, i);
        }
    }
    if(!found_identifier) {
        error = "REMOVE Error: identifier does not exist";
    }
    if(!strcmp(error, "")) {
        write_back_to_file(calendar, file_name);
    }
    int success = !strcmp(error, "");
    send_response_json(new_fd, action, calendar_name, identifier_value, success, error, cJSON_CreateObject());
    free(identifier);
    return;
}

void do_update(cJSON *calendar, int new_fd, char *file_name, char *action, char *calendar_name) {
    char fields[6][20] = {"date", "time", "duration", "name", "description", "location"};
    char *error = "";
    char *identifier = rec_data(new_fd, rec_data_sz(new_fd));
    char *field = rec_data(new_fd, rec_data_sz(new_fd));
    int valid_field = 0;
    for(int i = 0; i < sizeof(fields)/sizeof(fields[0]); i++) {
        if (!strcmp(fields[i], field)) {
            valid_field = 1;
        }
    }
    char *field_value = rec_data(new_fd, rec_data_sz(new_fd));
    if(!valid_field) {
        error = "UPDATE Error: invalid field value.";
    }
    cJSON *entry = NULL;
    int found_identifier = 0;
    cJSON_ArrayForEach(entry, calendar) {
        int curr_identifier = cJSON_GetNumberValue(cJSON_GetObjectItem(entry, "identifier"));
        if (atoi(identifier) == curr_identifier) {
            found_identifier = 1;
            cJSON_ReplaceItemInObject(entry, field, cJSON_CreateString(field_value));
        }
    }
    if(!found_identifier) {
        error = "UPDATE Error: identifier does not exist.";
    }
    if(!strcmp(error, "")) {
        write_back_to_file(calendar, file_name);
    }
    int success = !strcmp(error, "");
    send_response_json(new_fd, action, calendar_name, atoi(identifier), success, error, cJSON_CreateObject());
    free(identifier);
    free(field);
    free(field_value);
    return;
}

void do_getrange(cJSON *calendar, int new_fd, char *file_name, char *start_date, char *end_date, char *action, char *calendar_name) {
    int start_date_int = date_to_int(start_date);
    int end_date_int = date_to_int(end_date);
    int num_days = 0;
    cJSON *events_wrapper = cJSON_CreateObject();
    cJSON *events = cJSON_CreateArray();
    cJSON *entry = NULL;
    cJSON_ArrayForEach(entry, calendar) {
        cJSON *date = cJSON_GetObjectItem(entry, "date");
        char *date_str = cJSON_GetStringValue(date);
        int curr_date = date_to_int(date_str);
        if(curr_date >= start_date_int && curr_date <= end_date_int) {
            cJSON_AddItemReferenceToArray(events, entry);
            num_days += 1;
        } 
    }
    cJSON_AddNumberToObject(events_wrapper, "num_days", num_days);
    cJSON_AddItemToObject(events_wrapper, "events", events);
    //TODO return multiple identifiers??

    send_response_json(new_fd, action, calendar_name, 0, 1, "", events_wrapper);
    return;
}

void do_get(cJSON *calendar, int new_fd, char *file_name, char *action, char *calendar_name) {
    char *date = rec_data(new_fd, rec_data_sz(new_fd));
    do_getrange(calendar, new_fd, file_name, date, date, action, calendar_name);
    free(date);
    return;
}

void perform_action(char *action, cJSON *calendar, int new_fd, char *file_name, char *calendar_name) {
    if(!strcmp(action, "add")) {
        do_add(calendar, new_fd, file_name, "add", calendar_name);
    }
    else if(!strcmp(action, "remove")) {
        do_remove(calendar, new_fd, file_name, "remove", calendar_name);
    }
    else if(!strcmp(action, "update")) {
        do_update(calendar, new_fd, file_name, "update", calendar_name);
    }
    else if(!strcmp(action, "get")) {
        do_get(calendar, new_fd, file_name, "get", calendar_name);
    }
    else if(!strcmp(action, "getrange")) {
        char *start_date = rec_data(new_fd, rec_data_sz(new_fd));
        char *end_date = rec_data(new_fd, rec_data_sz(new_fd));
        do_getrange(calendar, new_fd, file_name, start_date, end_date, "getrange", calendar_name);
        free(start_date);
        free(end_date);
    }
    /*else if(!strcmp(action, "input")) {
        int sz = cJSON_GetArraySize(calendar);
        for (int i = 0; i < sz; i++) {
            char *str = cJSON_GetStringValue(cJSON_GetArrayItem(calendar, i)); // gets the string in the json
            char delim[] = " ";
            char *ptr = strtok(str, delim);
            char *arr[BUFSIZ];
            int j = 2;
            arr[0] = NULL;
            arr[1] = NULL;
	        while(ptr != NULL) {
                arr[j++] = ptr;    // place words of first string into array 
		        ptr = strtok(NULL, delim);
	        }
            if(!strcmp(arr[2], "add")) {
                do_add(calendar, new_fd, file_name, "add", calendar_name);
            }
            else if(!strcmp(arr[2], "remove")) {
                do_remove(calendar, new_fd, file_name, "remove", calendar_name);
            }
            else if(!strcmp(arr[2], "update")) {
                do_update(calendar, new_fd, file_name, "update", calendar_name);
            }
            else if(!strcmp(arr[2], "get")) {
                do_get(calendar, new_fd, file_name, "get", calendar_name);
            }
            else if(!strcmp(arr[2], "getrange")) {
                char *start_date = rec_data(new_fd, rec_data_sz(new_fd));
                char *end_date = rec_data(new_fd, rec_data_sz(new_fd));
                do_getrange(calendar, new_fd, file_name, start_date, end_date, "getrange", calendar_name);
                free(start_date);
                free(end_date);
            }
        }
    }*/

    else {
        send_response_json(new_fd, "INVALID", calendar_name, 0, 0, "ACTION: Invalid action", cJSON_CreateObject());
    }
    return;
}

#define BACKLOG 10   // how many pending connections queue will hold
void sigchld_handler(int s) {
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}
// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;
    if (argc < 2) {
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
        srand(time(NULL)); // for identifier
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family,get_in_addr((struct sockaddr *)&their_addr),s, sizeof s);
        printf("server: got connection from %s\n", s);
        if (!strcmp(argv[argc-1], "-mt")) {
            if (!fork()) { // this is the child process
                close(sockfd); // child doesn't need the listener
                char *calendar_name = rec_data(new_fd, rec_data_sz(new_fd));
                //open file
                char file_name[BUFSIZ];
                strcat(file_name, "server/data/");
                strcat(file_name, calendar_name);
                strcat(file_name, ".json");

                FILE *fp = fopen(file_name, "r");
                if(!fp) {
                    fp  = fopen(file_name, "w+");
                    char *br = "[]";
                    fputs(br, fp);
                }
                
                fseek(fp,0,SEEK_SET);
            
                //parse json file for calendar into cJSON object
                char calendar_buffer[BUFSIZ];
                if(fread(calendar_buffer, 1, BUFSIZ, fp) == -1) {
                    perror("unable to read calendar");
                    exit(1);
                }

                cJSON *calendar = cJSON_Parse(calendar_buffer);
                uint16_t num_actions;
                if(recv(new_fd, &num_actions, sizeof(num_actions), 0) == -1) {
                    perror("recv");
                    exit(1);
                }
                for(int i = 0; i < num_actions; i++) {
                    char *action = rec_data(new_fd, rec_data_sz(new_fd));
                    perform_action(action, calendar, new_fd, file_name, calendar_name);
                    free(action);
                }
                free(calendar_name);
                close(new_fd);
                exit(0); 
            }
        }
        else {
            char *calendar_name = rec_data(new_fd, rec_data_sz(new_fd));
            //open file
            char file_name[BUFSIZ];
            strcat(file_name, "server/data/");
            strcat(file_name, calendar_name);
            strcat(file_name, ".json");

            FILE *fp = fopen(file_name, "r");
            if(!fp) {
                fp  = fopen(file_name, "w+");
                char *br = "[]";
                fputs(br, fp);
            }

            fseek(fp,0,SEEK_SET);
           
            //parse json file for calendar into cJSON object
            char calendar_buffer[BUFSIZ];
            if(fread(calendar_buffer, 1, BUFSIZ, fp) == -1) {
                perror("unable to read calendar");
                exit(1);
            }

            cJSON *calendar = cJSON_Parse(calendar_buffer);
            uint16_t num_actions;
            if(recv(new_fd, &num_actions, sizeof(num_actions), 0) == -1) {
                perror("recv");
                exit(1);
            }
            for(int i = 0; i < num_actions; i++) {
                char *action = rec_data(new_fd, rec_data_sz(new_fd));
                perform_action(action, calendar, new_fd, file_name, calendar_name);
                free(action);
            }
            free(calendar_name);
            close(new_fd);
            exit(0);  
        }
        //close(new_fd);  // parent doesn't need this
    
    }
    return 0;
}