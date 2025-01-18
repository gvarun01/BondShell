#include "iman.h"

void remove_tags(char *str) {
    int in_tag = 0;
    char *p = str;

    while(*p){
        if(*p == '<'){
            in_tag = 1;
        }
        else if(*p == '>'){
            in_tag = 0;
        }
        else if(!in_tag){
            printf("%c", *p);
        }
        p++;
    }
}

void iman(char *subcom)
{
    
    char *comm = strtok(subcom, " \t\n");
    comm = strtok(NULL, " \t\n");
    trimstr(comm);

    if (comm == NULL)
    {
        // handleError("No command specified");
        printf("iMan: No command specified");
        return;
    }
    trimstr(comm);

    struct addrinfo hints, *res;
    int status, socketfd;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    status = getaddrinfo("man.he.net", "80", &hints, &res);
    if (status != 0)
    {
        // handleError("Could not get address info");
        printf("iMan: Could not get address info");
        return;
    }

    socketfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (socketfd == -1)
    {
        // handleError("Could not create socket");
        printf("iMan: Could not create socket");
        freeaddrinfo(res);
        return;
    }

    if (connect(socketfd, res->ai_addr, res->ai_addrlen) == -1)
    {
        // handleError("Could not connect to server");
        printf("iMan: Could not connect to server");
        close(socketfd);
        freeaddrinfo(res);
        return;
    }

    char request[1024];
    snprintf(request, sizeof(request), "GET /?topic=%s&section=0 HTTP/1.1\r\nHost: man.he.net\r\n\r\n", comm);

    if (send(socketfd, request, strlen(request), 0) == -1)
    {
        // handleError("Could not send request");
        printf("iMan: Could not send request");
        close(socketfd);
        freeaddrinfo(res);
        return;
    }

    char response[10001];
    char line[10001];
    int received, in_pre = 0;
    int it = 0;
    while ((received = recv(socketfd, response, 10000, 0)) > 0)
    {
        it++;
        response[received] = '\0';
        if (strstr(response, "No manual entry for") != NULL)
        {
            // handleError("No manual entry found");
            printf("iMan: No manual entry found");
            close(socketfd);
            freeaddrinfo(res);
            return;
        }

        if (it == 1)
        {
            char *firstbeg = strstr(response, "NAME");
            if (firstbeg != NULL)
            {
                char *secondbeg = strstr(firstbeg + 4, "NAME");
                if (secondbeg != NULL)
                {
                    remove_tags(secondbeg);
                }
                else
                    // printf("Second occurrence not found.\n");
                    handleError("iMan: Second occurrence not found");
            }
            else
                // printf("First occurrence not found.\n");
                handleError("iMan: First occurrence not found");
        }
        else{
            remove_tags(response);
        }
    }

    if (received == -1)
    {
        // handleError("Could not receive response");
        printf("iMan: Could not receive response");
        close(socketfd);
        freeaddrinfo(res);
        return;
    }

    close(socketfd);
    freeaddrinfo(res);
}