# AWS EduKit Example

> Based on [Core2-for-AWS-IoT-EduKit](https://github.com/m5stack/Core2-for-AWS-IoT-EduKit) and [AWS IoT EduKit Workshop](https://edukit.workshop.aws/en/).

![AWS IoT EduKit](/docs/edukit-box.jpg)

## Prerequisites

Install `v4.2` of [esp-idf](https://github.com/espressif/esp-idf).

```bash
$ > brew install cmake ninja dfu-util

$ > mkdir $HOME/esp
$ > cd $HOME/esp

$ > git clone -b release/v4.2 --recursive https://github.com/espressif/esp-idf.git
$ > cd esp-idf

$ > $HOME/esp/esp-idf/install.sh
```

## Configuration

```bash
# Configure esp-idf
$ > . $HOME/esp/esp-idf/export.sh

# Configure WiFi settings in "AWS Configuration" ( use "s" to save and "q" to quit )
$ > idf.py menuconfig
```

## Build

```bash
# Compile software
$ > idf.py build
```

## Flash

```bash
# Load software to Core2 and start monitor
$ > idf.py flash monitor
```

## Hints

Use `control + ]` to quit monitor.
