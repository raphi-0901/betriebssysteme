#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024

static void usage();

// Function to extract hostname and filepath from the URL
void parseUrl(const char *url, char *hostname, char *filepath) {
    char protocol[8];
    sscanf(url, "%7[^:]://%[^/]/%s", protocol, hostname, filepath);
    if (strcmp(protocol, "http") != 0) {
        fprintf(stderr, "Error: Only HTTP protocol is supported.\n");
        exit(EXIT_FAILURE);
    }

    if (filepath[strlen(filepath) - 1] == '\001' || filepath[strlen(filepath) - 1] == '\0') {
        strcpy(filepath, "index.html");
    }
        // Überprüfen, ob der String mit '/' endet
    else if (filepath[strlen(filepath) - 1] == '/') {
        // Anhängen von "index.html"
        strcat(filepath, "index.html");
    }
}

char *programName;

int main(int argc, char *argv[]) {
    programName = argv[0];


    // Parse command line arguments
    if (argc < 2) {
        usage();
    }

    // Parse options
    int opt;
    int port = 80;
    char *outputFile = NULL;

    bool fileSet = false;
    bool dirSet = false;

    while ((opt = getopt(argc, argv, "p:o:d:")) != -1) {
        switch (opt) {
            case 'p':
                char *endptr;
                long tempPort = strtol(optarg, &endptr, 10);

                if (endptr == optarg || *endptr != '\0') {
                    usage();
                }

                if (tempPort < 0 || tempPort > 65535) {
                    fprintf(stderr, "Error: Port number out of range (0-65535)\n");
                    usage();
                }

                port = (int) tempPort;
                break;
            case 'o':
                if (fileSet || dirSet) {
                    usage();
                }
                fileSet = true;
                outputFile = optarg;
                break;
            case 'd':
                if (fileSet || dirSet) {
                    usage();
                }
                dirSet = true;
                outputFile = optarg;
                break;
            default:
                usage();
        }
    }

    int sockfd;
    struct sockaddr_in serverAddr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];
    char hostname[256];
    char filepath[256];

    // Get URL
    const char *url = argv[optind];

    // Parse URL to extract hostname and filepath
    parseUrl(url, hostname, filepath);

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        return EXIT_FAILURE;
    }

    // Get server details using getaddrinfo
    struct addrinfo hints, *serverInfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(hostname, NULL, &hints, &serverInfo);
    if (status != 0) {
        fprintf(stderr, "Error getting address info: %s\n", gai_strerror(status));
        return EXIT_FAILURE;
    }

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        return EXIT_FAILURE;
    }

    // Fill in serverAddr structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr = ((struct sockaddr_in *) serverInfo->ai_addr)->sin_addr;

    // Connect to server
    if (connect(sockfd, (const struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
        perror("Error connecting");
        return EXIT_FAILURE;
    }

    freeaddrinfo(serverInfo); // Clean up

    // Create HTTP request
    char request[BUFFER_SIZE];
    sprintf(request, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", filepath, hostname);

    // Send HTTP request
    if (send(sockfd, request, strlen(request), 0) < 0) {
        perror("Error sending request");
        return EXIT_FAILURE;
    }

    // Receive and process HTTP response
    ssize_t bytesReceived;
    int statusCode = -1;

    FILE *outputStream = stdout;
    if (outputFile != NULL) {
        outputStream = fopen(outputFile, "w");
        if (outputStream == NULL) {
            fprintf(stderr, "Cannot open file: %s.\n", outputFile);
            exit(EXIT_FAILURE);
        }
    }

    while ((bytesReceived = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';

        // Check for the end of headers (empty line)
        char *body_start = strstr(buffer, "\r\n\r\n");
        if (body_start != NULL) {
            body_start += 4; // Move past the empty line
            fwrite(body_start, sizeof(char), bytesReceived - (body_start - buffer), outputStream);
        } else {
            fwrite(buffer, sizeof(char), bytesReceived, outputStream);
        }

        // Check response status code
        if (statusCode == -1) {
            char *status_line = strtok(buffer, "\r\n");
            if (strncmp(status_line, "HTTP/1.1", 8) == 0) {
                int parsedStatusCode = -1;
                sscanf(status_line, "%*s %d", &parsedStatusCode);
                statusCode = parsedStatusCode;
            } else {
                fprintf(stderr, "Protocol error!\n");
                return EXIT_FAILURE;
            }

            // Check if status is not 200 OK
            if (statusCode != 200) {
                fprintf(stderr, "Error: %d %s\n", statusCode, status_line + 9); // Printing status text
                return EXIT_FAILURE;
            }
        }
    }

    // Close socket and file
    close(sockfd);

    if (outputFile != NULL) {
        fclose(outputStream);
    }

    return EXIT_SUCCESS;
}

/**
 * Program usage info.
 * @brief Prints a usage info for the program and exits with EXIT_FAILURE
 * @return exits the programm with EXIT_FAILURE
 */
void usage() {
    fprintf(stderr, "Usage: %s [-p PORT] [-o FILE | -d DIR] URL\n", programName);
    exit(EXIT_FAILURE);
}
