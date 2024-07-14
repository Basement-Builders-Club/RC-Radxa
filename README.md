## Code to be ran on Radxa facilitating input and video transmission.

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
sudo ffmpeg -fflags nobuffer -fflags discardcorrupt -flags low_delay -re -f v4l2 -i /dev/video0 -preset ultrafast -tune zerolatency -c:v hevc -omit_video_pes_length 1 -rc_mode 0 -r 30 -level 30 -f rtsp rtsp://10.42.0.1:8554/stream

Desktop switch
CLI: systemctl set-default multi-user.target. 
GUI: systemctl set-default graphical.target

Other useful links to use
https://docs.radxa.com/en/zero/zero3/accessories/camera <- Camera setup
https://docs.radxa.com/en/zero/zero3/accessories/zero3w-antenna <- External antenna
https://docs.radxa.com/en/zero/zero3/app-development/gpiod <- GPIO usage
https://docs.radxa.com/en/zero/zero3/app-development/mraa <- MRAA usage
https://docs.radxa.com/en/zero/zero3/getting-started <- Getting started
