/**
 * @file client.c
 * @author Raphael Wirnsberger <e12220836@student.tuwien.ac.at>
 * @date 15.01.2023
 *
 * @brief Source file of server program of http.
 *
 **/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024
#define BACKLOG 10

char *programName;

void signalHandler(int signum);
void handleRequest(int clientSocket, const char *documentRoot, const char *indexFile);
void usage();

/**
 * Program entry point.
 * @details Serves the contents in a given folder.
 * @param argc The argument counter.
 * @param argv The optional arguments.
 * @return EXIT_SUCCESS or EXIT_FAILURE.
 */
int main(int argc, char *argv[])
{
    programName = argv[0];
    if (argc < 2)
    {
        usage();
    }

    // Default values
    int port = 8080;
    const char *indexFile = "index.html";
    const char *documentRoot;

    // Parse command-line arguments
    int opt;
    bool portAlreadySet = false;
    bool indexFileAlreadySet = false;
    while ((opt = getopt(argc, argv, "p:i:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            if (portAlreadySet)
            {
                usage();
            }
            portAlreadySet = true;

            char *endptr;
            long tempPort = strtol(optarg, &endptr, 10);

            if (endptr == optarg || *endptr != '\0')
            {
                usage();
            }

            if (tempPort < 0 || tempPort > 65535)
            {
                fprintf(stderr, "Error: Port number out of range (0-65535)\n");
                usage();
            }

            port = (int)tempPort;
            break;
        case 'i':
            if (indexFileAlreadySet)
            {
                usage();
            }
            indexFileAlreadySet = true;
            indexFile = optarg;
            break;
        default:
            usage();
        }
    }

    if (optind >= argc)
    {
        usage();
    }

    documentRoot = argv[optind];

    // Set up signal handlers
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Create socket
    int serverSocket, clientSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t addr_len = sizeof(clientAddress);

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(port);

    // Bind socket
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        perror("Binding failed");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    // Listen for connections
    if (listen(serverSocket, BACKLOG) == -1)
    {
        perror("Listen failed");
        close(serverSocket);
        return EXIT_FAILURE;
    }

    printf("Server listening on port %d\n", port);

    while (1)
    {
        // Accept incoming connections
        if ((clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &addr_len)) == -1)
        {
            perror("Accept failed");
            close(serverSocket);
            return EXIT_FAILURE;
        }

        printf("Connection from %s:%d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));

        // Handle request
        handleRequest(clientSocket, documentRoot, indexFile);
    }

    close(serverSocket);

    return EXIT_SUCCESS;
}

/**
 * Program usage info.
 * @brief Prints a usage info for the program and exits with EXIT_FAILURE
 * @return exits the programm with EXIT_FAILURE
 */
void usage()
{
    fprintf(stderr, "Usage: %s [-p PORT] [-o FILE | -d DIR] URL\n", programName);
    exit(EXIT_FAILURE);
}

void handleRequest(int clientSocket, const char *documentRoot, const char *indexFile)
{
    char buffer[BUFFER_SIZE];
    ssize_t bytesRead;
    char method[10], path[255], protocol[20];

    // Read the request from the client
    bytesRead = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytesRead < 0)
    {
        perror("Error reading from socket");
        close(clientSocket);
        return;
    }

    memset(method, 0, sizeof(method));
    memset(path, 0, sizeof(path));
    memset(protocol, 0, sizeof(protocol));

    // Parse the request
    sscanf(buffer, "%s %s %s", method, path, protocol);

    // Support only GET method
    if (strlen(method) == 0 || strlen(path) == 0 || strcmp(protocol, "HTTP/1.1") != 0)
    {
        char *response = "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n\r\n";
        send(clientSocket, response, strlen(response), 0);
        close(clientSocket);
        return;
    }

    // Support only GET method
    if (strcmp(method, "GET") != 0)
    {
        char *response = "HTTP/1.1 501 Not Implemented\r\nConnection: close\r\n\r\n";
        send(clientSocket, response, strlen(response), 0);
        close(clientSocket);
        return;
    }

    // Construct file path from request
    char fileRequested[255];
    memset(fileRequested, 0, sizeof(fileRequested));

    strcpy(fileRequested, path);
    if (path[strlen(path) - 1] == '/')
    {
        strcat(fileRequested, indexFile);
    }

    char filePath[255];
    snprintf(filePath, sizeof(filePath), "%s/%s", documentRoot, fileRequested);

    // Check if file exists
    int filefd = open(filePath, O_RDONLY);
    if (filefd < 0)
    {
        char *response = "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n";
        send(clientSocket, response, strlen(response), 0);
        close(clientSocket);
        return;
    }

    // Get file size
    struct stat st;
    stat(filePath, &st);
    off_t fileSize = st.st_size;

    // Send response headers
    char responseHeader[BUFFER_SIZE];
    time_t now = time(0);
    struct tm tm = *gmtime(&now);
    strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", &tm);
    snprintf(responseHeader, sizeof(responseHeader),
             "HTTP/1.1 200 OK\r\nDate: %s\r\nContent-Length: %ld\r\nConnection: close\r\n\r\n", buffer, fileSize);
    send(clientSocket, responseHeader, strlen(responseHeader), 0);

    // Send file content
    while ((bytesRead = read(filefd, buffer, BUFFER_SIZE)) > 0)
    {
        send(clientSocket, buffer, bytesRead, 0);
    }

    close(filefd);
    close(clientSocket);
}

void signalHandler(int signum)
{
    printf("Signal received. Closing server.\n");

    exit(0);
}