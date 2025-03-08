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
#define TRIGGER_MIN 0
#define TRIGGER_MAX 100
#define SERVO_MIN 0.975   //0.5ms
#define SERVO_MAX 0.875   //2.5ms
#define SERVO_PERIOD 20000
#define MOTOR_MIN 0.00
#define MOTOR_MAX 0.99
#define MOTOR_PERIOD 63
#define PACKET_CHAR_LEN 6
#define STEP 0.01

const int SERVO_PIN = 16;
const int MOTOR_PIN = 18;
float servo_duty = 0.925; //1.5ms (set to middle)
float motor_duty = 0.0;
mraa_pwm_context SERVO;
mraa_pwm_context MOTOR;
mraa_result_t result;
#define servo 1
#define motor 0

int Init_TCP (int *sock, struct sockaddr_in *serv_addr);
bool Read (int sock, float *wheel_angle, float *accelerator);
void* Thread_PWM (void* args);
int PWM_Init (int pin, int period, float duty, bool type);

bool rev = 0;
float vel = 0;
float mag = 0;

struct PWM_Args
{
  mraa_pwm_context context;
  float* input;
  float* duty_out;
  int input_min;
  int input_max;
  float duty_min;
  float duty_max;
};

int main()
{
  int sock = 0;
  float wheel_angle = 0;
  float accelerator = 0;
  struct sockaddr_in serv_addr;

  // Initialize TCP connection
  if (Init_TCP(&sock, &serv_addr) < 0) return -1;

  // Init PWM
  PWM_Init (SERVO_PIN, SERVO_PERIOD, servo_duty, servo);
  PWM_Init (MOTOR_PIN, MOTOR_PERIOD, motor_duty, motor);

  // Initialize PWM threads
  struct PWM_Args servo_args = {SERVO, &wheel_angle, &servo_duty, 
                                WHEEL_MIN, WHEEL_MAX, 
                                SERVO_MIN, SERVO_MAX};
  struct PWM_Args motor_args = {MOTOR, &mag, &motor_duty,
                                TRIGGER_MIN, TRIGGER_MAX,
                                MOTOR_MIN, MOTOR_MAX};

  pthread_t servo_thread;
  pthread_t motor_thread;

  pthread_create (&servo_thread, NULL, Thread_PWM, (void*) &servo_args);
  pthread_create (&motor_thread, NULL, Thread_PWM, (void*) &motor_args);

  // Main loop
  while (1) {
    // If read fails, continue
    if (!Read (sock, &wheel_angle, &accelerator))
        //break;

    printf ("Received Angle: %d\n", wheel_angle);
    printf ("Accelerator: %f\n", accelerator);
    printf ("Velocity: %f\n", vel);
    printf ("Magnitude: %f\n", mag);
    printf ("Servo Duty: %f\n", servo_duty);
    printf ("Motor Duty: %f\n", motor_duty);
  }

  // Close the socket
  close (sock);

  // Stop the PWM after running for the specified duration
  mraa_pwm_enable(SERVO, 0);
  mraa_pwm_enable(MOTOR, 0);

  // Close the PWM context
  mraa_pwm_close(SERVO);
  mraa_pwm_close(MOTOR);

  return 0;
}

// Read data from Unity
bool Read (int sock, float *wheel_angle, float *accelerator)
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
  *wheel_angle = atoi (token);

  // Get accelerator value
  token = strtok (NULL, ",");
  if (token == NULL) {
    printf ("Error: Missing trigger state\n");
    return false;
  }
  *accelerator = atoi (token);
  vel = vel + *accelerator * STEP;
  vel = vel > TRIGGER_MAX ? TRIGGER_MAX : vel;
  vel = vel < -1 * TRIGGER_MAX ? -1 * TRIGGER_MAX : vel;

  rev = vel < 0;
  mag = vel < 0 ? -1 * vel : vel;

  if (*accelerator == 0) mag = mag > 1 ? mag - 1 : 0; 
  vel = rev ? mag * -1: mag;

  printf ("Reverse: %i\n",rev);
  return true;
}

// Set PWM for GPIO
void Set_PWM (mraa_pwm_context context, float input, float* duty_out, 
              int input_min, int input_max, 
              float duty_min, float duty_max)
{
  // Calculate PWM duty cycle (0% to 100%)
  if (input > input_max) input = input_max;
  if (input < input_min) input = input_min;
  float duty_cycle = ((float) input - input_min) / (input_max - input_min);

  float duty = duty_min + duty_cycle * (duty_max - duty_min);
  *duty_out = duty;

  //set cycle
  mraa_pwm_write (context, duty);
}

// Thread function for PWM handling
void* Thread_PWM(void* args) {
  struct PWM_Args* pwm_args = (struct PWM_Args*) args;

  // Use the wheel_angle value for PWM control
  while (1) {
    Set_PWM (pwm_args->context, *(pwm_args->input), pwm_args->duty_out, 
             pwm_args->input_min, pwm_args->input_max, 
             pwm_args->duty_min, pwm_args->duty_max);
  }

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

int PWM_Init (int pin, int period, float duty, bool type){
  // Initialize PWM on the pin
  if (type == servo) {
    SERVO = mraa_pwm_init(pin);
    if (SERVO == NULL) {
      fprintf(stderr, "Failed to initialize PWM on pin %d\n", pin);
      return -1;
    }

    mraa_pwm_period_us(SERVO, period);
    // Set PWM duty cycle to 1.5 ms (1.5 ms / 20 ms = 0.075 or 7.5%)
    mraa_pwm_write(SERVO, duty);  // 7.5% duty cycle

    mraa_pwm_enable(SERVO, 1);
  }
  else if (type == motor) {
    MOTOR = mraa_pwm_init(pin);
    if (!MOTOR) {
      fprintf(stderr, "Failed to initialize PWM on pin %d\n", pin);
      return -1;
    }

    mraa_pwm_period_us(MOTOR, period);
    // Set PWM duty cycle to 1.5 ms (1.5 ms / 20 ms = 0.075 or 7.5%)
    mraa_pwm_write(MOTOR, duty);  // 7.5% duty cycle

    mraa_pwm_enable(MOTOR, 1);
  }

  return 0;
}
