/**
 * @file client.c
 * @author Raphael Wirnsberger <e12220836@student.tuwien.ac.at>
 * @date 15.01.2023
 *
 * @brief Source file of client program of http.
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>

#define BUFFER_SIZE 1024

static void usage();

bool isValidHTTPHeader(const char *response);

/**
 * Extract hostName and filePath from the url
 * @param url Url to parse.
 * @param hostName hostName to store at
 * @param filePath filePath to store at
 */
void parseUrl(const char *url, char *hostName, char *filePath)
{
    char protocol[8];
    sscanf(url, "%7[^:]://%[^;/?:@=&]%s", protocol, hostName, filePath);
    if (strcmp(protocol, "http") != 0)
    {
        fprintf(stderr, "Error: Only HTTP protocol is supported.\n");
        exit(EXIT_FAILURE);
    }

    if (filePath[0] == '/')
    {
        memmove(filePath, filePath + 1, strlen(filePath)); // Verschiebe den String um 1 nach links
    }
}

char *programName;

/**
 * Program entry point.
 * @details Fetches the content from the given url.
 * @param argc The argument counter.
 * @param argv The optional arguments.
 * @return EXIT_SUCCESS or EXIT_FAILURE.
 */
int main(int argc, char *argv[])
{
    programName = argv[0];

    // Parse command line arguments
    if (argc < 2)
    {
        usage();
    }

    // Parse options
    int opt;
    int port = 80;
    char *outputFile = NULL;
    char *outputDirectory = NULL;

    bool portAlreadySet = false;
    bool fileSet = false;
    bool dirSet = false;

    while ((opt = getopt(argc, argv, "p:o:d:")) != -1)
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
        case 'o':
            if (fileSet || dirSet)
            {
                usage();
            }
            fileSet = true;
            outputFile = optarg;
            break;
        case 'd':
            if (fileSet || dirSet)
            {
                usage();
            }
            dirSet = true;
            outputDirectory = optarg;

            // check if folder has no points in name
            const char *ptr = outputDirectory;
            while (*ptr)
            {
                if (*ptr == '.')
                {
                    fprintf(stderr, "Invalid folder name\n");
                    usage();
                    exit(EXIT_FAILURE);
                }
                ptr++;
            }

            break;
        default:
            usage();
        }
    }

    if ((argc - optind) != 1)
    {
        usage();
    }

    int sockfd;
    struct sockaddr_in serverAddr;
    char buffer[BUFFER_SIZE];
    char hostname[256];
    char filepath[256];
    memset(filepath, 0, sizeof(filepath));

    // Get URL
    const char *url = argv[optind];

    // Parse URL to extract hostname and filepath
    parseUrl(url, hostname, filepath);

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        fprintf(stderr, "Error opening socket\n");
        return EXIT_FAILURE;
    }

    // Get server details using getaddrinfo
    struct addrinfo hints, *serverInfo;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // Use IPv4
    hints.ai_socktype = SOCK_STREAM;

    int status = getaddrinfo(hostname, NULL, &hints, &serverInfo);
    if (status != 0)
    {
        fprintf(stderr, "Error getting address info: %s\n", gai_strerror(status));
        return EXIT_FAILURE;
    }

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Error opening socket");
        return EXIT_FAILURE;
    }

    // Fill in serverAddr structure
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr = ((struct sockaddr_in *)serverInfo->ai_addr)->sin_addr;

    // Connect to server
    if (connect(sockfd, (const struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        perror("Error connecting");
        return EXIT_FAILURE;
    }

    freeaddrinfo(serverInfo); // Clean up

    // Create HTTP request
    char request[BUFFER_SIZE];
    sprintf(request, "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n", filepath, hostname);

    // Send HTTP request
    if (send(sockfd, request, strlen(request), 0) < 0)
    {
        perror("Error sending request");
        return EXIT_FAILURE;
    }

    // Receive and process HTTP response
    ssize_t bytesReceived;
    int statusCode = -1;

    FILE *outputStream = stdout;
    if (dirSet)
    {
        char outputPath[strlen(outputDirectory) + 256 + 1];
        if (outputDirectory != NULL)
        {
            strcpy(outputPath, outputDirectory);
            if (filepath[0] == '\0' || filepath[0] == '?')
            {
                strcat(outputPath, "/index.html");
            }
            else if (filepath[0] != '/')
            {
                strcat(outputPath, "/");
            }
            strcat(outputPath, filepath);
        }

        if (mkdir(outputDirectory, 0777) != 0)
        {
            if (errno != EEXIST)
            {
                fprintf(stderr, "Cannot create mkdir: %s.\n", outputDirectory);
                exit(EXIT_FAILURE);
            }
        }

        outputStream = fopen(outputPath, "w+");
        if (outputStream == NULL)
        {
            fprintf(stderr, "Cannot open file: %s.\n", outputPath);
            exit(EXIT_FAILURE);
        }
    }
    else if (fileSet)
    {
        outputStream = fopen(outputFile, "w+");
        if (outputStream == NULL)
        {
            fprintf(stderr, "Cannot open file: %s.\n", outputFile);
            exit(EXIT_FAILURE);
        }
    }

    while ((bytesReceived = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0)
    {
        buffer[bytesReceived] = '\0';

        // Check for the end of headers (empty line)
        char *body_start = strstr(buffer, "\r\n\r\n");
        if (body_start != NULL)
        {
            body_start += 4; // Move past the empty line
            fwrite(body_start, sizeof(char), bytesReceived - (body_start - buffer), outputStream);
        }
        else
        {
            fwrite(buffer, sizeof(char), bytesReceived, outputStream);
        }

        // Check response status code
        if (statusCode == -1)
        {
            char *statusLine = strtok(buffer, "\r\n");
            if (isValidHTTPHeader(statusLine))
            {
                int parsedStatusCode = -1;
                sscanf(statusLine, "%*s %d", &parsedStatusCode);
                statusCode = parsedStatusCode;
            }
            else
            {
                fprintf(stderr, "Protocol error!\n");
                exit(2);
            }

            // Check if status is not 200
            if (statusCode != 200)
            {
                fprintf(stderr, "%s\n", statusLine + 9); // Printing status text
                exit(3);
            }
        }
    }

    // Close socket and file
    close(sockfd);

    if (outputFile != NULL)
    {
        fclose(outputStream);
    }

    exit(EXIT_SUCCESS);
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

/**
 * Checks the HTTP header.
 * @return true if the http header is valid
 */
bool isValidHTTPHeader(const char *response)
{
    // Überprüfe, ob der Header mit "HTTP/1.1 " beginnt
    if (strncmp(response, "HTTP/1.1 ", 9) != 0)
    {
        return false;
    }

    // Versuche, die Zahl nach "HTTP/1.1 " zu parsen
    char *endptr;
    strtol(response + 9, &endptr, 10);

    // Überprüfe, ob strtol erfolgreich war und ob das Ende des Strings erreicht wurde
    if (endptr == response + 9)
    {
        return false;
    }

    return true;
}
