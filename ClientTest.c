#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 256

int main() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // Create socket, AF_INET for IPV6, SOCK_STREAM for TCP, Protocol Value:
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, getenv("IPV4"), &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf("Connected to the server\n");

    while (1) {
        printf("Waiting for data...\n");
        // Read data from the server
        valread = read(sock, buffer, BUFFER_SIZE);
        if (valread > 0) {
            // Null-terminate the received string
            buffer[valread] = '\0';
            printf("Received raw data: %s\n", buffer);

            // Convert the received string to a float and print it
            float angle = atof(buffer);
            printf("Received Angle: %f\n", angle);
        } else if (valread < 0) {
            perror("Read error");
            break;
        } else {
            printf("Connection closed by server\n");
            break;
        }
    }

    // Close the socket
    close(sock);

    return 0;
}