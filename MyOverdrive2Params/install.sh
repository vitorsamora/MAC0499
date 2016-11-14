#!/bin/bash

path=$(find /usr -name 'lv2' | grep '/lib/lv2')
echo "LV2 path in your filesystem: $path"

echo "Configuring..."
./waf configure
echo "Building..."
./waf
echo "Copying..."
sudo cp -avr ./build/*.lv2 $path/