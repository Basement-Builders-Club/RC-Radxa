## TODO
Combine RC_Motor & RC_Servo

Isolate core on Radxa to run RC.c isolated from OS and other programs

Fix Unity (currently controllers not working properly)

Build

## Code to be ran on Radxa facilitating input and video transmission.
RC.c is the current working file.

## Interface info
Running PoseClient after starting the scene will run a test client

Up arrow and down arrow control trigger forward/backward in the XR sim

Trigger 0, 1, and 2 mean backward, still, and forward respectively.

## General
TCP Listener (Server) is run from Unity

TCP Client (Client) is RC.c, or the RC executable

Make sure that inbound firewalls are open for Unity on the computer running it.

Connect to the Radxa access point from the server computer, and use ipconfig to find the IP that is used in RC.c.

## WSL info:

wsl

gcc -o RC RC.c -lgpiod

./RC

Update IP or set IPV4 environment var and recompile before running

telnet [IPV4] 8080

Turn firewall off (?)

## Radxa info:

Turn on host wifi, "AP" is designated name of connection (https://docs.radxa.com/en/zero/zero3/radxa-os/ap <- Shell tab)

sudo nmcli connection up AP

FFMPEG video stream command (FFMPEG must be compiled and set up correctly before hand https://docs.radxa.com/en/zero/zero3/app-development/rtsp)

nohup ./mediamtx &

sudo ffmpeg -fflags nobuffer -fflags discardcorrupt -flags low_delay -re -f v4l2 -i /dev/video0 -preset ultrafast -tune zerolatency -c:v hevc -omit_video_pes_length 1 -rc_mode 0 -r 30 -level 30 -f rtsp rtsp://<IP>:8554/stream

(10.42.0.1 (Direct) or 192.168.1.35)

FFPLAY (Client side, install FFMPEG through "choco install ffmpeg" in windows terminal [install chocolatey if needed])

ffplay -probesize 32 -analyzeduration 0 -sync ext -fflags nobuffer -fflags discardcorrupt -flags low_delay -tune zerolatency -flags2 fast -preset ultrafast -framedrop -avioflags direct -strict experimental -rtsp_transport tcp rtsp://<IP>:8554/stream

(10.42.0.1 (Direct) or 192.168.1.35)

Desktop switch

CLI: systemctl set-default multi-user.target.

GUI: systemctl set-default graphical.target

Compilation:

gcc -o [file] [file].c -lgpiod -lpthread -O3 -march=armv8.2-a -mtune=cortex-a55 -ffast-math -fomit-frame-pointer

sudo chrt -f 99 ./[file]

Set CPU governor to perfomance & both min and max freq to 1.42GHz to avoid undefined behavior through cpufreq

sudo apt-get install cpufrequtils

sudo nano /etc/default/cpufrequtils

GOVERNOR="performance"

MIN_SPEED="1.42GHz"

MAX_SPEED="1.42GHz"

sudo systemctl enable cpufrequtils

sudo systemctl restart cpufrequtils

## Testing info:
For motor testing:

- Go by wire colors

white is PWM

orange -> yellow is brake

expects 12V, freq from 4khz - 16khz, digital low is on, high is off. Full range pwm/


For servo:

Expects 5v, 20ms period (50hz), expects duty cycle from .5ms - 2.5ms

## Other links/hardware
Camera setup: https://docs.radxa.com/en/zero/zero3/accessories/camera

External antenna: https://docs.radxa.com/en/zero/zero3/accessories/zero3w-antenna

GPIO usage: https://docs.radxa.com/en/zero/zero3/app-development/gpiod

MRAA usage: https://docs.radxa.com/en/zero/zero3/app-development/mraa

Getting started: https://docs.radxa.com/en/zero/zero3/getting-started
