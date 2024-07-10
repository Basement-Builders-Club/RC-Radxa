// todo:
// video output
// trigger input for gas

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
//#include <gpiod.h>

#define PORT 8080
#define BUFFER_SIZE 256
#define WHEEL_MIN 0
#define WHEEL_MAX 360

int InitTCP(int *sock, struct sockaddr_in *serv_addr);
//int InitGPIO(struct gpiod_chip *chip, struct gpiod_line *lineLED, struct gpiod_line *lineButton);
int Read(int sock, float *wheelAngle, bool *accelerator);
//int SetLED(int wheelAngle, gpiod_line *lineLED);

int main() {
    int sock = 0;
    float wheelAngle;
    bool accelerator;
    struct sockaddr_in serv_addr;
    //struct gpiod_chip *chip;
    //struct gpiod_line *lineLED;
    //struct gpiod_line *lineButton;

    // Initialize TCP connection
    if (InitTCP(&sock, &serv_addr) < 0) {
        return -1;
    }
    //InitGPIO(chip, lineLED, lineButton);

    // Main loop
    while (1) {
        if (!Read(sock, &wheelAngle, &accelerator)) {
            break;
        }
        //int PWM = SetLED(wheelAngle, lineLED);
        printf("Received Angle: %f\n", wheelAngle);
        printf("Accelerator: %d\n", accelerator);
        //printf("PWM: %d\n", PWM);
    }

    // Close the socket
    close(sock);

    // Close GPIO
    /*
    gpiod_line_release(lineLED);
    gpiod_line_release(lineButton);
    gpiod_chip_close(chip);
    */

    return 0;
}

int InitTCP(int *sock, struct sockaddr_in *serv_addr) {
    const char *ip = getenv("IPV4");
    if (ip == NULL) {
        fprintf(stderr, "Environment variable IPV4 is not set\n");
        return -1;
    }

    // Create socket, AF_INET for IPV4, SOCK_STREAM for TCP, Protocol Value:
    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr->sin_family = AF_INET;
    serv_addr->sin_port = htons(PORT);

    // Convert IPv4 addresses from text to binary form
    if (inet_pton(AF_INET, ip, &serv_addr->sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    // Connect to the server
    if (connect(*sock, (struct sockaddr*)serv_addr, sizeof(*serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf("Connected to the server\n");
    return 0;
}

/*
Initializes the GPIO
Sets chip, linLED, and lineButton pointers
*/
/*
int InitGPIO(struct gpiod_chip *chip, struct gpiod_line *lineLED, struct gpiod_line *lineButton) {
    // GPIO Init
    const char *chipname = "gpiochip3";

    chip = gpiod_chip_open_by_name(chipname);
    lineLED = gpiod_chip_get_line(chip, 1);
    lineButton = gpiod_chip_get_line(chip, 2);

    gpiod_line_request_output(lineLED,"out", 0);
    gpiod_line_request_input(lineButton, "in");

    return 1;
}
*/

/*
Reads data from Unity
*/
int Read(int sock, float *wheelAngle, bool *accelerator) {
    int valread;
    char buffer[BUFFER_SIZE] = {0};

    printf("Waiting for data...\n");
    
    // Read data from the server
    valread = read(sock, buffer, BUFFER_SIZE);
    if (valread > 0) {
        // Null-terminate the received string
        buffer[valread] = '\0';
        printf("Received raw data: %s\n", buffer);

        // Split the received message into angle and trigger state
        // Get the first token in the data
        char* token = strtok(buffer, ",");
        if (token != NULL) {
            // Set it to wheel angle
            *wheelAngle = atof(token);

            // Get the next token
            token = strtok(NULL, ",");
            if (token != NULL) {
                // set it to accelerator value
                *accelerator = atoi(token);
            } else {
                printf("Error: Missing trigger state\n");
                return 0; // Indicate error
            }
        } else {
            printf("Error: Invalid data format\n");
            return 0; // Indicate error
        }
    } else if (valread < 0) {
        perror("Read error");
        return 0; // Indicate error
    } else {
        printf("Connection closed by server\n");
        return 0; // Indicate connection closed
    }

    return 1; // Indicate successful read
}

/*
Sets LEDs on GPIO
*/
/*
int SetLED(int wheelAngle, gpiod_line *lineLED) {
    // Calculate PWM duty cycle (0% to 100%)
    int duty_cycle = (wheelAngle - WHEEL_MIN) / (WHEEL_MAX - WHEEL_MIN);

    // Start PWM sequence
    gpiod_line_set_value(lineLED, 1); //on

    // Simulate PWM operation (adjust as needed for actual GPIO operation)
    int PWM = duty_cycle * 1000;
    usleep(duty_cycle * 1000); // Sleep proportional to duty cycle (in milliseconds)

    // End PWM sequence
    gpiod_line_set_value(lineLED, 0); //off

    return PWM;
}
*/