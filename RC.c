#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <gpiod.h>
#include <pthread.h>

#define PORT 8080
#define BUFFER_SIZE 256
#define WHEEL_MIN 0
#define WHEEL_MAX 360

// Declare the mutex globally
pthread_mutex_t lock;

int InitTCP(int *sock, struct sockaddr_in *serv_addr);
int InitGPIO(struct gpiod_chip **chip, struct gpiod_line **lineLED, struct gpiod_line **lineButton);
int Read(int sock, float *wheelAngle, bool *accelerator);
bool SetLED(int wheelAngle, struct gpiod_line *lineLED, int* ledValue);
void* threadLED(void* args);

struct LEDArgs {
    float* wheelAngle;
    struct gpiod_line* lineLED;
    int* ledValue;
};

int main() {
    int sock = 0;
    float wheelAngle = 0;
    bool accelerator = false;
    int ledValue = 0;
    struct sockaddr_in serv_addr;
    struct gpiod_chip *chip;
    struct gpiod_line *lineLED;
    struct gpiod_line *lineButton;

    // Initialize mutex
    pthread_mutex_init(&lock, NULL);

    // Initialize TCP connection
    if (InitTCP(&sock, &serv_addr) < 0) {
        return -1;
    }
    InitGPIO(&chip, &lineLED, &lineButton);

    // Initialize LED PWM thread
    struct LEDArgs ledArgs = {&wheelAngle, lineLED, &ledValue};
    pthread_t ledThread;
    pthread_create(&ledThread, NULL, threadLED, (void*) &ledArgs);

    // Main loop
    while (1) {
        // Lock mutex before updating the wheel angle
        pthread_mutex_lock(&lock);

        if (!Read(sock, &wheelAngle, &accelerator)) {
            pthread_mutex_unlock(&lock);
            break;
        }

        printf("Received Angle: %f\n", wheelAngle);
        printf("Accelerator: %d\n", accelerator);
        printf("LED: %i\n", ledValue);

        // Unlock mutex after updating the wheel angle
        pthread_mutex_unlock(&lock);

        // Sleep for 100ms to let CPU chill
        usleep(100000);
    }

    // Close the socket
    close(sock);

    // Close GPIO
    gpiod_line_release(lineLED);
    gpiod_line_release(lineButton);
    gpiod_chip_close(chip);

    // Destroy mutex
    pthread_mutex_destroy(&lock);

    return 0;
}

// Thread function for LED handling
void* threadLED(void* args) {
    struct LEDArgs* ledArgs = (struct LEDArgs*) args;
    while (1) {
        // Lock mutex before accessing the wheel angle
        pthread_mutex_lock(&lock);

        // Use the wheelAngle value for LED control
        SetLED(*ledArgs->wheelAngle, ledArgs->lineLED, ledArgs->ledValue);

        // Unlock mutex after reading the wheel angle
        pthread_mutex_unlock(&lock);
    }
    return NULL;
}

// Initialize TCP connection
int InitTCP(int *sock, struct sockaddr_in *serv_addr) {
    const char *ip = getenv("IPV4"); // alternatively hardcode in your IPV4

    if (ip == NULL) {
        fprintf(stderr, "Environment variable IPV4 is not set\n");
        return -1;
    }

    if ((*sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr->sin_family = AF_INET;
    serv_addr->sin_port = htons(PORT);

    if (inet_pton(AF_INET, ip, &serv_addr->sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }

    if (connect(*sock, (struct sockaddr*)serv_addr, sizeof(*serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf("Connected to the server\n");
    return 0;
}

// Initialize GPIO
int InitGPIO(struct gpiod_chip **chip, struct gpiod_line **lineLED, struct gpiod_line **lineButton) {
    const char *chipname = "gpiochip3";

    *chip = gpiod_chip_open_by_name(chipname);
    *lineLED = gpiod_chip_get_line(*chip, 1);
    *lineButton = gpiod_chip_get_line(*chip, 2);

    gpiod_line_request_output(*lineLED, "out", 0);
    gpiod_line_request_input(*lineButton, "in");

    return 1;
}

// Read data from Unity
int Read(int sock, float *wheelAngle, bool *accelerator) {
    int valread;
    char buffer[BUFFER_SIZE] = {0};

    printf("Waiting for data...\n");

    valread = read(sock, buffer, BUFFER_SIZE);
    if (valread > 0) {
        buffer[valread] = '\0';
        printf("Received raw data: %s\n", buffer);

        char* token = strtok(buffer, ",");
        if (token != NULL) {
            *wheelAngle = atof(token);

            token = strtok(NULL, ",");
            if (token != NULL) {
                *accelerator = atoi(token);
            } else {
                printf("Error: Missing trigger state\n");
                return 0;
            }
        } else {
            printf("Error: Invalid data format\n");
            return 0;
        }
    } else if (valread < 0) {
        perror("Read error");
        return 0;
    } else {
        printf("Connection closed by server\n");
        return 0;
    }

    return 1;
}

// Set LEDs on GPIO
bool SetLED(int wheelAngle, struct gpiod_line *lineLED, int* ledValue) {
     // Calculate PWM duty cycle (0% to 100%)
    if (wheelAngle > WHEEL_MAX) wheelAngle = WHEEL_MAX;
    if (wheelAngle < WHEEL_MIN) wheelAngle = WHEEL_MIN;
    float duty_cycle = ((float) wheelAngle - WHEEL_MIN) / (WHEEL_MAX - WHEEL_MIN);

    //gpiod_line_set_value(lineLED, 1); //on
    int PWM = duty_cycle * 1000;
    *ledValue = PWM;

    //on
    gpiod_line_set_value(lineLED, 1);
    usleep(PWM);
    //off
    gpiod_line_set_value(lineLED, 0);
    usleep(1001 - PWM);
}
