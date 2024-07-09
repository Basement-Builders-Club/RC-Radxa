#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <gpiod.h>

int main() {
    int ch;

    // Initialize ncurses
    initscr();
    cbreak(); // Disable line buffering and echo
    noecho(); // Turn off echoing of characters

    // GPIO Init
    const char *chipname = "gpiochip3";
    struct gpiod_chip *chip;
    struct gpiod_line *lineLED;
    struct gpiod_line *lineButton;
    int out, val;

    chip = gpiod_chip_open_by_name(chipname);
    lineLED = gpiod_chip_get_line(chip, 1);
    lineButton = gpiod_chip_get_line(chip, 2);

    gpiod_line_request_output(lineLED,"out", 0);
    gpiod_line_request_input(lineButton, "in");

    // Set getch() to non-blocking
    nodelay(stdscr, TRUE);

    // Loop indefinitely to poll for keys
    while (1) {
        ch = getch(); // Poll for a key press
        printf("Hello, World!\n");
        if (ch != ERR) { // If a key was pressed
                if (ch >= '0' && ch <= '9') {
                    int num = ch - '0'; // Convert char to integer
                    // Calculate PWM duty cycle (0% to 100%)
                    int duty_cycle = num * 10;

                    // Start PWM sequence
                    gpiod_line_set_value(lineLED, 1); //on

                    // Simulate PWM operation (adjust as needed for actual GPIO operation)
                    usleep(duty_cycle * 1000); // Sleep proportional to duty cycle (in milliseconds)

                    // End PWM sequence
                    gpiod_line_set_value(lineLED, 0); //off

                } else if (ch == 27) { // 27 is ASCII code for ESC key
                    break; // Exit loop on ESC key press
                }
            } else {
                gpiod_line_set_value(lineLED, 0); //off
            }
        }

    // Cleanup ncurses
    endwin();

    // Close GPIO
    gpiod_line_release(lineLED);
    gpiod_line_release(lineButton);
    gpiod_chip_close(chip);

    return 0;
}
        