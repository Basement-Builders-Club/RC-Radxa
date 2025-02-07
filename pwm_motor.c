#include <stdio.h>
#include <unistd.h>
#include <mraa/pwm.h>

int main() {
    const int pin = 18;  // set pin number
    float duty = 0.5; 
    #define MAX 0.975
    #define MIN 0.875
    #define STEP 0.00001
    mraa_result_t result;

    // Initialize PWM on the pin
    mraa_pwm_context pwm = mraa_pwm_init(pin);

    // Set PWM period to 50 Hz (20 ms)
    mraa_pwm_period_us(pwm, 63);  // 20 ms = 50 Hz

    // Set PWM duty cycle to 1.5 ms (1.5 ms / 20 ms = 0.075 or 7.5%)
    mraa_pwm_write(pwm, 0.5);  // 7.5% duty cycle

    // Start the PWM
    mraa_pwm_enable(pwm, 1);

    // Run for 10 seconds, then stop (adjust duration as needed)
    float add = STEP;
    while(1){
        // duty += add;
        // if (duty > MAX - STEP) add = -STEP;
        // if (duty < MIN + STEP) add = STEP;
        //printf("duty: %f\n", duty);
        mraa_pwm_write(pwm, duty);
    }

    // Stop the PWM after running for the specified duration
    mraa_pwm_enable(pwm, 0);

    // Close the PWM context
    mraa_pwm_close(pwm);

    return 0;
}
s