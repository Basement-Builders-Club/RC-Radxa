#include <stdio.h>
#include <unistd.h>
#include <mraa/pwm.h>

int main() {
    const int pin = 16;  // set pin number
    const int pin2 = 18;  // set pin number
    mraa_result_t result;

    // Initialize PWM on the pin
    mraa_pwm_context pwm = mraa_pwm_init(pin);
    mraa_pwm_context pwm2 = mraa_pwm_init(pin2);
    // Stop the PWM after running for the specified duration
    mraa_pwm_enable(pwm, 0);
    mraa_pwm_enable(pwm2, 0);

    // Close the PWM context
    mraa_pwm_close(pwm);
    mraa_pwm_close(pwm2);

    return 0;
}
