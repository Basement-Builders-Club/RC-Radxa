## Code to be ran on Radxa facilitating input and video transmission.

## WSL info:

wsl

g++ -g FileName.c -o FileName.out

./FileName.[exe/out]

gcc KeyPoll.c -lcurses -lgpiod

Update IP and recompile before running

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

Other useful links to use
https://docs.radxa.com/en/zero/zero3/accessories/camera <- Camera setup
https://docs.radxa.com/en/zero/zero3/accessories/zero3w-antenna <- External antenna
https://docs.radxa.com/en/zero/zero3/app-development/gpiod <- GPIO usage
https://docs.radxa.com/en/zero/zero3/app-development/mraa <- MRAA usage
https://docs.radxa.com/en/zero/zero3/getting-started <- Getting started
