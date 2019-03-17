#!/bin/bash
sleep 5 
while true; do
	sleep 0.5
	gst-launch-1.0 v4l2src device='/dev/v4l/by-path/platform-tegra-xhci-usb-0:3.1:1.0-video-index0' ! video/x-raw, width=640, height=480 ! omxh264enc control-rate=2 bitrate=4000000 ! video/x-h264, stream-format=byte-stream ! h264parse ! rtph264pay mtu=1400 ! udpsink host=127.0.0.1 clients=10.34.76.54:5801 port=5801 sync=false async=false
done
