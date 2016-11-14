MAC0499: Final Course Assignment - Realtime Audio Effects
==============================

This repository contains some LV2 audio plugins developed for a final course assignment in Institute of Mathematics ans Statistics - University of São Paulo.

## Dependencies

Since the installation is made via
[waf](http://waf.io/book/) the only requirement to install the plugins and the
[LV2](http://lv2plug.in/) interface is to have Python installed.

## Installation

There's a script to install each plugin. To install a plugin just go to its directory and run:
```
sudo ./install.sh
```
It may take a while since there's a find command to discover where is the LV2 path in your filesystem.

Alternatively, you can build the bundles of the plugins by waf and move the bundle directory to the LV2 path on your filesystem. To build, just run the commands:
```
./waf configure
./waf
```
