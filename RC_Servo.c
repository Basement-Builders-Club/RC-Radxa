#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <mraa/pwm.h>
#include <stdbool.h>


#define PORT 8080
#define BUFFER_SIZE 256
#define WHEEL_MIN 0
#define WHEEL_MAX 360
#define SERVO_MIN 0.975   //0.5ms
#define SERVO_MAX 0.875   //2.5ms
#define SERVO_PERIOD 20000
#define MOTOR_MIN 0.01
#define MOTOR_MAX 0.99
#define MOTOR_PERIOD 63
#define PACKET_CHAR_LEN 5
const int SERVO_PIN = 16;
const int MOTOR_PIN = 18;
float servo_duty = 0.925; //1.5ms (set to middle)
float motor_duty = 0.0; //off
mraa_pwm_context SERVO;
mraa_pwm_context MOTOR;
mraa_result_t result;
#define servo 1
#define motor 0

int Init_TCP (int *sock, struct sockaddr_in *serv_addr);
bool Read (int sock, int *wheelAngle, bool *accelerator);
void* Thread_PWM (void* args);
int PWM_Init(int pin, int period, float duty, bool type);

struct PWM_Args
{
    int* wheelAngle;
    int* PWM_value;
};

int main()
{
    int sock = 0;
    int wheelAngle = 0;
    bool accelerator = false;
    int PWM_value = 0;
    struct sockaddr_in serv_addr;

    // Initialize TCP connection
    if (Init_TCP(&sock, &serv_addr) < 0) return -1;

    // Init PWM
    PWM_Init(SERVO_PIN, SERVO_PERIOD, servo_duty, servo);

    // Initialize PWM thread
    struct PWM_Args pwmArgs = {&wheelAngle, &PWM_value};
    pthread_t pwmThread;
    pthread_create (&pwmThread, NULL, Thread_PWM, (void*) &pwmArgs);

    // Main loop
    while (1) {
        // If read fails, continue
        if (!Read (sock, &wheelAngle, &accelerator))
            //break;

        printf ("Received Angle: %d\n", wheelAngle);
        printf ("Accelerator: %d\n", accelerator);
        printf ("PWM: %i\n", PWM_value);
    }

    // Close the socket
    close (sock);

    // Stop the PWM after running for the specified duration
    mraa_pwm_enable(SERVO, 0);
    //mraa_pwm_enable(MOTOR, 0);

    // Close the PWM context
    mraa_pwm_close(SERVO);
    //mraa_pwm_close(MOTOR);

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

/*
#define SERVO_MIN 0.975   //0.5ms
#define SERVO_MAX 0.875   //2.5ms
#define SERVO_PERIOD 20000
*/

// Set PWM for GPIO
bool Set_PWM (int wheelAngle, int* PWM_value)
{
    // Calculate PWM duty cycle (0% to 100%)
    if (wheelAngle > WHEEL_MAX) wheelAngle = WHEEL_MAX;
    if (wheelAngle < WHEEL_MIN) wheelAngle = WHEEL_MIN;
    float duty_cycle = ((float) wheelAngle - WHEEL_MIN) / (WHEEL_MAX - WHEEL_MIN);

    float duty = duty_cycle * (SERVO_MIN - SERVO_MAX) + SERVO_MAX;
    *PWM_value = duty;

    //set cycle
    mraa_pwm_write(SERVO, duty);
    printf("Duty: %f\n", duty);
}

// Thread function for PWM handling
void* Thread_PWM(void* args) {
    struct PWM_Args* pwmArgs = (struct PWM_Args*) args;

    // Use the wheelAngle value for PWM control
    while (1)
        Set_PWM(*(pwmArgs->wheelAngle), pwmArgs->PWM_value);

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

int PWM_Init(int pin, int period, float duty, bool type){
    // Initialize PWM on the pin
    if(type == servo) {
        SERVO = mraa_pwm_init(pin);
         // Set PWM period 
        mraa_pwm_period_us(SERVO, period);
        // Set PWM duty cycle to 1.5 ms (1.5 ms / 20 ms = 0.075 or 7.5%)
        mraa_pwm_write(SERVO, duty);  // 7.5% duty cycle

        mraa_pwm_enable(SERVO, 1);
    }
    else if(type == motor) {
        MOTOR = mraa_pwm_init(pin);
        // Set PWM period 
        mraa_pwm_period_us(MOTOR, period);
        // Set PWM duty cycle to 1.5 ms (1.5 ms / 20 ms = 0.075 or 7.5%)
        mraa_pwm_write(MOTOR, duty);  // 7.5% duty cycle

        mraa_pwm_enable(MOTOR, 1);
    }

}