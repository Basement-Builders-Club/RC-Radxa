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
#define DUTY_CYCLE_PERIOD_MICROSEC 20000
#define PACKET_CHAR_LEN 5

int Init_TCP (int *sock, struct sockaddr_in *serv_addr);
int Init_GPIO (struct gpiod_chip **chip, struct gpiod_line **linePWM, struct gpiod_line **lineButton);
bool Read (int sock, int *wheelAngle, bool *accelerator);
bool Set_PWM (int wheelAngle, struct gpiod_line *linePWM, int* PWMValue);
void* Thread_PWM (void* args);

struct PWMArgs
{
    int* wheelAngle;
    struct gpiod_line* linePWM;
    int* PWMValue;
};

int main()
{
    int sock = 0;
    int wheelAngle = 0;
    bool accelerator = false;
    int PWMValue = 0;
    struct sockaddr_in serv_addr;
    struct gpiod_chip *chip;
    struct gpiod_line *linePWM;
    struct gpiod_line *lineButton;

    // Initialize TCP connection
    if (Init_TCP(&sock, &serv_addr) < 0)
        return -1;
    
    Init_GPIO(&chip, &linePWM, &lineButton);

    // Initialize PWM thread
    struct PWMArgs pwmArgs = {&wheelAngle, linePWM, &PWMValue};
    pthread_t pwmThread;
    pthread_create (&pwmThread, NULL, Thread_PWM, (void*) &pwmArgs);

    // Main loop
    while (1) {
        // If read fails, continue
        if (!Read (sock, &wheelAngle, &accelerator))
            break;

        printf ("Received Angle: %d\n", wheelAngle);
        printf ("Accelerator: %d\n", accelerator);
        printf ("PWM: %i\n", PWMValue);
    }

    // Close the socket
    close (sock);

    // Close GPIO
    gpiod_line_release (linePWM);
    gpiod_line_release (lineButton);
    gpiod_chip_close (chip);

    return 0;
}

// Read data from Unity
bool Read (int sock, int *wheelAngle, bool *accelerator)
{
    int valread;
    char buffer[BUFFER_SIZE] = {0};

    printf ("Waiting for data...\n");

    valread = read (sock, buffer, BUFFER_SIZE);
    
    // If didnt read, print this to terminal
    if (valread < 0)
        printf ("Read error");
    // Check for packet completeness
    if (valread < sizeof (char) * PACKET_CHAR_LEN)
        printf ("Incomplete packet\n");

    buffer[valread] = '\0';
    printf ("Received raw data: %s\n", buffer);

    // Check for ~ sentinel value
    char* token = strtok (buffer, ",");
    if (token == NULL || strcmp (token, "~") != 0) {
        printf ("Error: Invalid data format\n");
        return false;
    }

    // Get wheel angle
    token = strtok (NULL, ",");
    if (token == NULL) {
        printf ("Error: Missing wheel angle\n");
        return false;
    }
    *wheelAngle = atoi (token);

    // Get accelerator value
    token = strtok (NULL, ",");
    if (token == NULL) {
        printf ("Error: Missing trigger state\n");
        return false;
    }
    *accelerator = atoi (token);

    return true;
}

// Set PWM for GPIO
bool Set_PWM (int wheelAngle, struct gpiod_line *linePWM, int* PWMValue)
{
    // Calculate PWM duty cycle (0% to 100%)
    if (wheelAngle > WHEEL_MAX) wheelAngle = WHEEL_MAX;
    if (wheelAngle < WHEEL_MIN) wheelAngle = WHEEL_MIN;
    float duty_cycle = ((float) wheelAngle - WHEEL_MIN) / (WHEEL_MAX - WHEEL_MIN);

    int PWM = duty_cycle * DUTY_CYCLE_PERIOD_MICROSEC;
    *PWMValue = PWM;

    //on
    gpiod_line_set_value (linePWM, 1);
    usleep (PWM);
    //off
    gpiod_line_set_value (linePWM, 0);
    usleep (DUTY_CYCLE_PERIOD_MICROSEC + 1 - PWM);
}

// Thread function for PWM handling
void* Thread_PWM(void* args) {
    struct PWMArgs* pwmArgs = (struct PWMArgs*) args;

    // Use the wheelAngle value for PWM control
    while (1)
        Set_PWM(*(pwmArgs->wheelAngle), pwmArgs->linePWM, pwmArgs->PWMValue);

    return NULL;
}

/*********** INIT FUNCS *************/

// Initialize TCP connection
int Init_TCP (int *sock, struct sockaddr_in *serv_addr)
{
    const char *ip = "10.42.0.171"; // alternatively hardcode in your IPV4

    // IP validity check
    if (ip == NULL) {
        fprintf (stderr, "Environment variable IPV4 is not set\n");
        return -1;
    }

    if ((*sock = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        perror ("Socket creation error");
        return -1;
    }

    serv_addr->sin_family = AF_INET;
    serv_addr->sin_port = htons (PORT);

    if (inet_pton (AF_INET, ip, &serv_addr->sin_addr) <= 0) {
        perror ("Invalid address/ Address not supported");
        return -1;
    }

    if (connect (*sock, (struct sockaddr*) serv_addr, sizeof (*serv_addr)) < 0) {
        perror ("Connection Failed");
        return -1;
    }

    printf ("Connected to the server\n");
    return 0;
}

// Initialize GPIO
int Init_GPIO(struct gpiod_chip **chip, struct gpiod_line **linePWM, struct gpiod_line **lineButton)
{
    const char *chipname = "gpiochip3";

    *chip = gpiod_chip_open_by_name (chipname);
    *linePWM = gpiod_chip_get_line (*chip, 1);
    *lineButton = gpiod_chip_get_line (*chip, 2);

    gpiod_line_request_output (*linePWM, "out", 0);
    gpiod_line_request_input (*lineButton, "in");

    return 1;
}
