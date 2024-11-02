#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <gpiod.h>
#include <pthread.h>
#include <sched.h>

#define PORT 8080
#define BUFFER_SIZE 256
#define WHEEL_MIN 0
#define WHEEL_MAX 360

pthread_mutex_t angleLock;  // Declare the mutex globally

int InitTCP(int *sock, struct sockaddr_in *serv_addr);
int InitGPIO(struct gpiod_chip **chip, struct gpiod_line **lineLED, struct gpiod_line **lineButton);
int Read(int sock, int *wheelAngle, bool *accelerator);
bool SetLED(int* wheelAngle, struct gpiod_line *lineLED, int* ledValue);
void* threadLED(void* args);

struct LEDArgs {
    int* wheelAngle;
    struct gpiod_line* lineLED;
    int* ledValue;
};

int main() {
    int sock = 0;
    int wheelAngle = 0;
    bool accelerator = false;
    int ledValue = 0;
    struct sockaddr_in serv_addr;
    struct gpiod_chip *chip;
    struct gpiod_line *lineLED;
    struct gpiod_line *lineButton;

    // Initialize the mutex
    pthread_mutex_init(&angleLock, NULL);

    // Initialize TCP connection
    if (InitTCP(&sock, &serv_addr) < 0) {
        return -1;
    }
    InitGPIO(&chip, &lineLED, &lineButton);

    // Initialize LED PWM thread
    struct LEDArgs ledArgs = {&wheelAngle, lineLED, &ledValue};
    pthread_t ledThread;

    // Set attributes for the PWM thread
    pthread_attr_t pwm_attr;
    pthread_attr_init(&pwm_attr);
    pthread_attr_setschedpolicy(&pwm_attr, SCHED_FIFO); // Set to real-time scheduling policy

    struct sched_param pwm_param;
    pwm_param.sched_priority = 90;  // Set high priority for the PWM thread
    pthread_attr_setschedparam(&pwm_attr, &pwm_param);

    // Create the PWM thread with high priority
    pthread_create(&ledThread, &pwm_attr, threadLED, (void*) &ledArgs);

    // Main loop
    while (1) {
        // Lock mutex before updating the wheel angle
        pthread_mutex_lock(&angleLock);

        if (!Read(sock, &wheelAngle, &accelerator)) {
            pthread_mutex_unlock(&angleLock);  // Unlock mutex before breaking the loop
            break;
        }

        printf("Received Angle: %d\n", wheelAngle);
        printf("Accelerator: %d\n", accelerator);
        printf("LED: %i\n", ledValue);

        // Unlock mutex after updating the wheel angle
        pthread_mutex_unlock(&angleLock);

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
    pthread_mutex_destroy(&angleLock);

    return 0;
}

// Thread function for LED handling
void* threadLED(void* args) {
    struct LEDArgs* ledArgs = (struct LEDArgs*) args;
    while (1) {
        // Use the wheelAngle value for LED control
        SetLED(ledArgs->wheelAngle, ledArgs->lineLED, ledArgs->ledValue);
    }
    return NULL;
}

// Initialize TCP connection
int InitTCP(int *sock, struct sockaddr_in *serv_addr) {
    const char *ip = "10.42.0.171"; // alternatively hardcode in your IPV4

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
int Read(int sock, int *wheelAngle, bool *accelerator) {
    int valread;
    char buffer[BUFFER_SIZE] = {0};

    printf("Waiting for data...\n");

    valread = read(sock, buffer, BUFFER_SIZE);
    if (valread > 0) {
        buffer[valread] = '\0';
        printf("Received raw data: %s\n", buffer);

        char* token = strtok(buffer, ",");
        if (token != NULL) {
            token = strtok(NULL, ",");
            pthread_mutex_lock(&angleLock);
            *wheelAngle = atoi(token);
            pthread_mutex_unlock(&angleLock);

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
bool SetLED(int* wheelAngle, struct gpiod_line *lineLED, int* ledValue) {
    // Calculate PWM duty cycle (0% to 100%)
    pthread_mutex_lock(&angleLock);
    int angle = *wheelAngle;
    if (angle > WHEEL_MAX) angle = WHEEL_MAX;
    if (angle < WHEEL_MIN) angle = WHEEL_MIN;
    float duty_cycle = ((float) angle - WHEEL_MIN) / (WHEEL_MAX - WHEEL_MIN);
    pthread_mutex_unlock(&angleLock);

    int PWM = duty_cycle * 100;
    *ledValue = PWM;

    int on_time = PWM > 0 ? PWM : 1;  // Ensures minimal on-time for stability
    int off_time = (101 - PWM) > 0 ? (101 - PWM) : 1; // Minimal off-time

    // on
    gpiod_line_set_value(lineLED, 1);
    usleep(on_time);

    // off
    gpiod_line_set_value(lineLED, 0);
    usleep(off_time);
}
