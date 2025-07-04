#include "../include/iman.h"
#include "../utils/error_handler.h" // For handleError
#include "../utils/string_utils.h"  // For trimstr
#include <stdio.h>      // For printf, snprintf
#include <string.h>     // For strtok, strstr, strlen, memset
#include <stdlib.h>     // For freeaddrinfo
#include <unistd.h>     // For close, send, recv
#include <sys/socket.h> // For socket, connect
#include <netdb.h>      // For getaddrinfo, struct addrinfo

// Removes HTML-like tags from a string and prints the content.
// This function prints char by char, which can be inefficient.
// A buffered approach or building a new string would be better for performance
// but for man page sizes, this might be acceptable.
static void remove_tags_and_print(const char *str) { // Made static, takes const char*
    if (str == NULL) return;

    bool in_tag = false; // Use bool for clarity
    const char *p = str;

    while(*p){
        if(*p == '<'){
            in_tag = true;
        }
        else if(*p == '>'){
            in_tag = false;
            // Consume the '>' itself, don't print it
        }
        else if(!in_tag){
            printf("%c", *p);
        }
        p++;
    }
}

void iman(char *full_iman_command) // Renamed subcom for clarity
{
    if (full_iman_command == NULL) {
        handleError("iMan: NULL command string passed.");
        return;
    }
    // Make a copy for strtok, as it modifies the string
    char *command_copy = strdup(full_iman_command);
    if (command_copy == NULL) {
        handleError("iMan: strdup failed for command copy.");
        return;
    }

    char *first_token = strtok(command_copy, " \t\n"); // Should be "iMan"
    if (first_token == NULL || strcmp(first_token, "iMan") != 0) {
        // This case should ideally not happen if dispatch is correct
        handleError("iMan: Command not correctly dispatched or malformed.");
        free(command_copy);
        return;
    }

    char *topic_name = strtok(NULL, " \t\n"); // Get the actual topic for the man page

    if (topic_name == NULL || strlen(topic_name) == 0)
    {
        // Original code printed directly, using handleError is more consistent for errors.
        handleError("iMan: No command topic specified. Usage: iMan <command>");
        free(command_copy);
        return;
    }
    // No need to trimstr(topic_name) as strtok handles surrounding spaces.

    struct addrinfo hints, *resolved_addresses;
    int gai_status, socket_fd; // Renamed status to gai_status, socketfd to socket_fd

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;       // Use IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP socket

    gai_status = getaddrinfo("man.he.net", "80", &hints, &resolved_addresses);
    if (gai_status != 0)
    {
        // Use gai_strerror for more descriptive error messages from getaddrinfo
        char err_msg[200];
        snprintf(err_msg, sizeof(err_msg), "iMan: getaddrinfo failed: %s", gai_strerror(gai_status));
        handleError(err_msg);
        free(command_copy);
        return;
    }

    // Create socket
    socket_fd = socket(resolved_addresses->ai_family, resolved_addresses->ai_socktype, resolved_addresses->ai_protocol);
    if (socket_fd == -1)
    {
        perror("iMan: socket creation failed"); // Use perror for system call errors
        freeaddrinfo(resolved_addresses);
        free(command_copy);
        return;
    }

    // Connect to server
    if (connect(socket_fd, resolved_addresses->ai_addr, resolved_addresses->ai_addrlen) == -1)
    {
        perror("iMan: connect failed");
        close(socket_fd);
        freeaddrinfo(resolved_addresses);
        free(command_copy);
        return;
    }

    // Construct HTTP GET request
    char http_request[1024];
    snprintf(http_request, sizeof(http_request), "GET /?topic=%s&section=all HTTP/1.1\r\nHost: man.he.net\r\nConnection: close\r\n\r\n", topic_name);

    // Send request
    if (send(socket_fd, http_request, strlen(http_request), 0) == -1)
    {
        perror("iMan: send failed");
        close(socket_fd);
        freeaddrinfo(resolved_addresses);
        free(command_copy);
        return;
    }

    // Receive and process response
    char response_buffer[10001]; // Buffer for each recv call
    int bytes_received;
    bool content_started = false; // Flag to indicate if we are past HTTP headers

    // A more robust way to find content: search for "\r\n\r\n"
    // For simplicity, the old logic of "NAME...NAME" is kept for the first chunk, then direct print.
    // This can be improved significantly.

    bool first_chunk = true;
    while ((bytes_received = recv(socket_fd, response_buffer, sizeof(response_buffer) - 1, 0)) > 0)
    {
        response_buffer[bytes_received] = '\0'; // Null-terminate the received chunk

        if (strstr(response_buffer, "No manual entry for") != NULL)
        {
            char err_msg[100];
            snprintf(err_msg, sizeof(err_msg), "iMan: No manual entry found for '%s'.", topic_name);
            handleError(err_msg);
            // Cleanup handled at the end
            break;
        }

        char *content_to_print = response_buffer;
        if (first_chunk) {
            char *name_section_start = strstr(response_buffer, "NAME");
            if (name_section_start) {
                // Find the second occurrence of "NAME" to skip the first "NAME" line itself if it's part of a header.
                // This logic is a bit fragile as it depends on the exact HTML structure of man.he.net
                char *actual_content_start = strstr(name_section_start + strlen("NAME"), "NAME");
                if (actual_content_start) {
                    content_to_print = actual_content_start;
                } else {
                    // If second "NAME" not found, maybe the first one is already the content.
                    // Or, the structure is different. For now, proceed with name_section_start.
                    content_to_print = name_section_start;
                }
            } else {
                // If "NAME" not found in the first chunk, it might be an error page or different format.
                // Try to find where the actual content starts (e.g., after headers)
                char *header_end = strstr(response_buffer, "\r\n\r\n");
                if (header_end) {
                    content_to_print = header_end + 4; // Skip past \r\n\r\n
                }
                // else, print the whole buffer for now, might contain headers.
            }
            first_chunk = false;
        }
        remove_tags_and_print(content_to_print);
    }

    if (bytes_received == -1)
    {
        perror("iMan: recv failed");
        // Cleanup handled at the end
    }
    printf("\n"); // Add a newline after the man page content

    // Cleanup
    close(socket_fd);
    freeaddrinfo(resolved_addresses);
    free(command_copy);
}