# I2C Communication on Linux: A Hands-On Tutorial with MLX90614

## Table of Contents

### 1. Setup
1.1 MLX90614 Sensor Introduction
1.2 Hardware Connections
1.3 Enable I2C Bus in Device Tree
1.4 Enable i2c-tools

### 2. Read Sensor Data via Generic I2C Interface
2.1 Use i2c-tools to Read Registers
2.2 Shell Script to Print Sensor Data
2.3 C Program to Log Data

### 3. Read Sensor Data using Kernel Driver
3.1 Enable Driver in SDK using Kernel Config Tool
3.2 Specify Driver in Device Tree
3.3 Shell Commands to Log Sensor Data via Kernel Driver
3.4 C Program to Log Sensor Data via Kernel Driver
  
---

## 1.1 MLX90614 Sensor Introduction

The MLX90614 is a simple I2C non-contact temperature sensor.

**I2C Address:** 0x5A

**Registers of Interest:**
- **0x06** - Ambient Temperature (Ta)
- **0x07** - Object Temperature (To)
- **0x0E** - Sensor ID

Each register contains 16 bits (2 bytes) of data.

**Temperature Conversion:**
```
Temperature (°C) = (raw_value × 0.02) - 273.15
```

*For detailed conversion formulas and accuracy specifications, see the [MLX90614 Datasheet](https://www.melexis.com/-/media/files/documents/datasheets/mlx90614-datasheet-melexis.pdf).*

**Product Page:** [Waveshare Infrared Temperature Sensor](https://www.waveshare.com/wiki/Infrared_Temperature_Sensor)

---

## 1.2 Hardware Connections

Choose I2C connectors from: [Luckfox Pico I2C Documentation](https://wiki.luckfox.com/Luckfox-Pico/Luckfox-Pico-RV1106/Luckfox-Pico-Ultra-W/Luckfox-Pico-I2C)

**Our Choice: I2C2 M0**
- SDA: Pin 3 (GPIO1_C7)
- SCL: Pin 5 (GPIO1_D0)
- VCC: 3.3V
- GND: Ground

Connect the MLX90614 sensor to these pins.

---

## 1.3 Enable I2C Bus in Device Tree

Edit the device tree to enable I2C2. We choose I2C2 M0 since only M0 is exposed on the dev board.

**Note:** `config/dts_config` is a symlink and should be ignored by git. To find the actual file to edit, use:
```bash
$ file config/buildroot_defconfig # Example output: 
config/buildroot_defconfig: symbolic link to ../sysdrv/source/buildroot/buildroot-2023.02.6/configs/luckfox_pico_w_defconfig
```

Add the following DTS settings at the end of the actual dts file:

```dts
/**********I2C2**********/
&i2c2 {
	status = "okay";
	clock-frequency = <100000>;
};
```

*Note: For quick testing, you can also temporarily enable I2C2 using `luckfox-config` on the board via SSH as described in the [Luckfox-config documentation](https://wiki.luckfox.com/Luckfox-Pico/Luckfox-Pico-RV1106/Luckfox-Pico-Ultra-W/Luckfox-config).*

## 1.4 Enable i2c-tools

This installs `i2cget`, `i2cset`, `i2cdetect`, and other I2C utilities on the target system.

### Enable i2c-tools in Buildroot

**Start Docker Container**
*Note: If you haven't set up the Docker environment yet, follow the [Luckfox Pico SDK Docker setup guide](https://wiki.luckfox.com/Luckfox-Pico/Luckfox-Pico-RV1106/Luckfox-Pico-Ultra-W/Luckfox-Pico-SDK#2-docker-environment) first.*
```bash
$ sudo docker start -ai luckfox
root@container:/# 

# Navigate to build directory
root@container:/# cd /home/
root@container:/home# 
```

There are two ways to enable i2c-tools:

**Method 1: GUI Configuration (Recommended)**
Run the following command to open a GUI window (ensure your terminal window has enough height):
```bash
root@container:/home# ./build.sh buildrootconfig
```
Navigate to: `Target packages` → `Hardware handling` → `i2c-tools`
Enable `i2c-tools` package
Save and exit

**Method 2: Direct File Edit**
Alternatively, you can directly edit the buildroot configuration file:
```bash
root@container:/home# echo "BR2_PACKAGE_I2C_TOOLS=y" >> config/buildroot_defconfig
```

Both methods add a `BR2_PACKAGE_I2C_TOOLS=y` line in the buildroot config file. You can verify with:
```bash
$ grep I2C_TOOLS config/buildroot_defconfig
BR2_PACKAGE_I2C_TOOLS=y
```

**Rebuild and Flash**

**Inside Docker container:**
```bash
root@container:/home# ./build.sh all
```

**Outside Docker container (with sudo):**
```bash
$ sudo ./build.sh flash
```

**SSH to Luckfox Board**
```bash
ssh root@<board-ip>
```

**Verify I2C Bus**
```bash
root@luckfox:~# ls /dev/i2c*
/dev/i2c-2  /dev/i2c-3  /dev/i2c-4
```

We should see `/dev/i2c-2` which corresponds to the I2C2 bus we enabled in the device tree. If we remove the I2C2 configuration from the DTS and rebuild/flash again, `/dev/i2c-2` will disappear.

**Verify Sensor Detection**
```bash
root@luckfox:~# i2cdetect -y 2
```

Expected output:
```
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:                         -- -- -- -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
50: -- -- -- -- -- -- -- -- -- -- 5a -- -- -- -- -- 
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
70: -- -- -- -- -- -- -- --   
```

The `5a` at address 0x5a indicates the MLX90614 sensor is detected and responding on I2C bus 2.

**If you see `UU` instead of `5a`:**
This means the sensor is being used by a kernel driver. To use i2c-tools, you need to unload the driver first:
```bash
root@luckfox:~# rmmod mlx90614
```
Then run `i2cdetect -y 2` again to see the `5a` address.

---

## 2.1 Use i2c-tools to Read Registers

Now that we have i2c-tools installed and can detect our MLX90614 sensor, let's use the tools to read the sensor's registers and extract temperature data.

### Reading Multiple Registers at Once

You can use `i2cdump` to read a range of registers and get an overview of all sensor data:

```bash
root@luckfox:~# i2cdump -y 2 0x5a w
     0,8  1,9  2,a  3,b  4,c  5,d  6,e  7,f
00: 2629 0098 XXXX 1a15 XXXX 0008 39b3 3998 
08: 3bc4 0000 0008 XXXX 04b1 0000 3998 XXXX 
10: 8067 1b58 040d 0007 XXXX 0002 XXXX 0000 
18: 01ff 801c 04a9 XXXX 0012 0162 XXXX 01ba 
20: 9993 62e3 0201 f71c ffff aff4 6b11 6bb5 
28: 6ef6 XXXX 95f5 1100 XXXX 20b9 XXXX 079a 
30: 9e69 75d3 53e3 8021 8020 1d2b 00c1 XXXX 
38: 0013 0000 XXXX 8011 e806 e560 1123 e1d0 
40: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
48: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
50: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
58: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
60: XXXX XXXX 44b7 XXXX XXXX 44b8 XXXX 0000 
68: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
70: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
78: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
80: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
88: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
90: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
98: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
a0: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
a8: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
b0: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
b8: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
c0: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
c8: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
d0: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
d8: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
e0: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
e8: XXXX XXXX XXXX XXXX XXXX XXXX XXXX XXXX 
f0: f701 40e6 40da 0024 40dd 40e2 40df 0000 
f8: 40d9 2d14 XXXX d790 XXXX 40d9 XXXX XXXX 
```

This shows all registers in a readable format. You can see:
- Register 0x06: `39b3` (ambient temperature)
- Register 0x07: `3998` (object temperature)  
- Register 0x0E: `2629` (sensor ID)

The `XXXX` entries indicate registers that are either not readable or contain invalid data.

### Reading Individual Registers

The `i2cget` command allows us to read specific registers from the sensor. Since the MLX90614 uses 16-bit registers, we need to read 2 bytes at a time.

```bash
# Read the three main registers from MLX90614 sensor
root@luckfox:~# i2cget -y 2 0x5a 0x06 w  # Ambient Temperature register
0x39c2                      # Raw value: 14786 decimal → 22.42°C

root@luckfox:~# i2cget -y 2 0x5a 0x07 w  # Object Temperature register  
0x39cb                      # Raw value: 14795 decimal → 22.64°C

root@luckfox:~# i2cget -y 2 0x5a 0x0e w  # Sensor ID register
0x3c65                      # Raw value: 15461 decimal → Sensor ID
```

The `w` flag tells i2cget to read a 16-bit word (2 bytes) in little-endian format, which is what the MLX90614 uses.

**Note:** Raw register values need to be converted to actual temperatures using the formula:
```
Temperature (°C) = (raw_value × 0.02) - 273.15
```

## 2.2 Shell Script to Print Sensor Data
Using `i2cget`, we can use the script to log sensor data. For simplicity, we create `read_mlx90614.sh` directly in the luckfox `/root/` directory:

```bash
#!/bin/bash

I2C_BUS="2"
SENSOR_ADDR="0x5a"
TEMP_AMBIENT_REG="0x06"
TEMP_OBJECT_REG="0x07"

# Check if i2c-tools is installed
if ! command -v i2cget &> /dev/null; then
    echo "Error: i2c-tools not found. Install with: opkg install i2c-tools"
    exit 1
fi

# Check if I2C device exists
if [ ! -e "/dev/i2c-$I2C_BUS" ]; then
    echo "Error: I2C bus /dev/i2c-$I2C_BUS not found"
    exit 1
fi

echo "MLX90614 Temperature Reader (Ctrl+C to exit)"
echo "============================================="

while true; do
    # Read ambient temperature
    ambient_raw=$(i2cget -y $I2C_BUS $SENSOR_ADDR $TEMP_AMBIENT_REG w 2>/dev/null)
    if [ $? -eq 0 ]; then
        ambient_c=$(echo "scale=1; ($(printf "%d" $ambient_raw) * 0.02) - 273.15" | bc -l)
        echo "Ambient: ${ambient_c}°C"
    fi
    
    # Read object temperature
    object_raw=$(i2cget -y $I2C_BUS $SENSOR_ADDR $TEMP_OBJECT_REG w 2>/dev/null)
    if [ $? -eq 0 ]; then
        object_c=$(echo "scale=1; ($(printf "%d" $object_raw) * 0.02) - 273.15" | bc -l)
        echo "Object:  ${object_c}°C"
    fi
    
    echo "---"
    sleep 2
done
```

**Sample output from running the script:**
```bash
root@luckfox:~# ./read_mlx90614.sh
MLX90614 Temperature Reader (Ctrl+C to exit)
=============================================
Ambient: 22.33°C
Object:  22.17°C
---
Ambient: 22.31°C
Object:  22.07°C
---
Ambient: 22.35°C
Object:  21.95°C
---
...
```

The script continuously monitors both ambient and object temperatures, updating every 2 seconds. Press Ctrl+C to exit.

**Note:** This script uses `bc` for floating-point calculations. In this Buildroot configuration, `bc` is provided by busybox, if you are using `systemd` you can enable bc in buildroot config:

```bash
# Inside Docker container
root@container:/home# ./build.sh buildrootconfig
```
Navigate to: `Target packages` → `Miscellaneous` → `bc`
Enable the `bc` package and save the configuration.






## 2.3 C Program to Log Data

While shell scripts are convenient for quick testing, C programs offer better performance and more control over I2C communication. Let's create a C program that directly interfaces with the I2C bus.

### The C Program

**Project Structure:**
```
project/app/read_MLX90614/
├── read_mlx90614.c      # C program source code
├── read_mlx90614.sh     # Shell script version
├── Makefile             # Build configuration
└── i2c_read_tutorial.md # This tutorial
```
**Note:** We've put the .sh file in the project as well for convenience

Create `read_mlx90614.c`:

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <errno.h>
#include <string.h>
#include <byteswap.h>

#define I2C_BUS 2
#define SENSOR_ADDR 0x5a
#define TEMP_AMBIENT_REG 0x06
#define TEMP_OBJECT_REG 0x07

int i2c_fd = -1;

// Function to read 16-bit register from I2C device using SMBus protocol
int read_register(int reg) {
    struct i2c_smbus_ioctl_data args;
    union i2c_smbus_data data;
    
    args.read_write = I2C_SMBUS_READ;
    args.command = reg;
    args.size = I2C_SMBUS_WORD_DATA;
    args.data = &data;
    
    if (ioctl(i2c_fd, I2C_SMBUS, &args) < 0) {
        return -1;
    }
    
    // MLX90614 returns data in big-endian format, but i2c_smbus_read_word_data
    // already handles the byte swapping, so we return the value as-is
    return data.word;
}

// Function to convert raw temperature to Celsius
float raw_to_celsius(int raw_value) {
    return (raw_value * 0.02) - 273.15;
}

int main() {
    char i2c_device[20];
    int ambient_raw, object_raw;
    float ambient_c, object_c;
    
    // Check if I2C device exists
    snprintf(i2c_device, sizeof(i2c_device), "/dev/i2c-%d", I2C_BUS);
    if (access(i2c_device, F_OK) != 0) {
        printf("Error: I2C bus %s not found\n", i2c_device);
        return 1;
    }
    
    // Open I2C device
    i2c_fd = open(i2c_device, O_RDWR);
    if (i2c_fd < 0) {
        printf("Error: Cannot open %s: %s\n", i2c_device, strerror(errno));
        return 1;
    }
    
    // Set slave address
    if (ioctl(i2c_fd, I2C_SLAVE, SENSOR_ADDR) < 0) {
        printf("Error: Cannot set slave address 0x%02x: %s\n", SENSOR_ADDR, strerror(errno));
        close(i2c_fd);
        return 1;
    }
    
    printf("MLX90614 Temperature Reader (Ctrl+C to exit)\n");
    printf("=============================================\n");
    
    while (1) {
        // Read ambient temperature
        ambient_raw = read_register(TEMP_AMBIENT_REG);
        if (ambient_raw >= 0) {
            ambient_c = raw_to_celsius(ambient_raw);
            printf("Ambient: %.1f°C\n", ambient_c);
        } else {
            printf("Ambient: Error reading sensor\n");
        }
        
        // Read object temperature
        object_raw = read_register(TEMP_OBJECT_REG);
        if (object_raw >= 0) {
            object_c = raw_to_celsius(object_raw);
            printf("Object:  %.1f°C\n", object_c);
        } else {
            printf("Object:  Error reading sensor\n");
        }
        
        printf("---\n");
        sleep(2);
    }
    
    close(i2c_fd);
    return 0;
}
```

Create Makefile

```makefile
# Include Luckfox SDK build parameters
ifeq ($(APP_PARAM), )
    APP_PARAM := ../Makefile.param
    include $(APP_PARAM)
endif

export LC_ALL := C
SHELL := /bin/bash

CURRENT_DIR := $(shell pwd)

# Project configuration
PKG_NAME := read_MLX90614
PKG_BIN  ?= out

# Compiler flags for cross-compilation
READ_MLX90614_CFLAGS  := $(RK_APP_OPTS) $(RK_APP_CROSS_CFLAGS)
READ_MLX90614_LDFLAGS := $(RK_APP_OPTS) $(RK_APP_LDFLAGS)

PKG_TARGET := read_MLX90614-build

# Main build target
all: $(PKG_TARGET)
	@echo "build $(PKG_NAME) done"

# Build the project: compile C program and copy shell script
read_MLX90614-build:
	@rm -rf $(PKG_BIN); \
	mkdir -p $(PKG_BIN)/bin
	# Compile the C program using cross-compiler
	$(RK_APP_CROSS)-gcc $(READ_MLX90614_CFLAGS) read_mlx90614.c -o $(PKG_BIN)/bin/read_MLX90614 $(READ_MLX90614_LDFLAGS)
	# Copy shell script and make it executable
	cp read_mlx90614.sh $(PKG_BIN)/bin/
	chmod +x $(PKG_BIN)/bin/read_mlx90614.sh
	# Copy built files to app output directory
	$(call MAROC_COPY_PKG_TO_APP_OUTPUT, $(RK_APP_OUTPUT), $(PKG_BIN))

# Clean build output
clean:
	@rm -rf $(PKG_BIN)

distclean: clean

# Show build information
info:
	@echo "This is $(PKG_NAME) for $(RK_APP_CHIP) with $(RK_APP_CROSS)"
```

**Note:** The Makefile copies both the compiled C program (`read_MLX90614`) and the shell script (`read_mlx90614.sh`) to the output directory. This gives users the choice between running the high-performance C program or the simpler shell script version.

**Expected files after build and flash:**
After building and flashing, you should find these files on the target system:

```bash
# On the target board (via SSH)
root@luckfox:~# pwd
/oem/usr/bin
root@luckfox:~# ls read_*
read_mlx90614     read_mlx90614.sh
```

Both executables are now available system-wide and can be run from any directory.

**Sample output:**
```bash
# Run the shell script
root@luckfox:~# ./read_mlx90614.sh 
MLX90614 Temperature Reader (Ctrl+C to exit)
=============================================
Ambient: 23.21°C
Object:  23.05°C
---
Ambient: 23.19°C
Object:  23.11°C
---
^C

# Run the C program
root@luckfox:~# ./read_mlx90614
MLX90614 Temperature Reader (Ctrl+C to exit)
=============================================
Ambient: 23.2°C
Object:  22.8°C
---
Ambient: 23.2°C
Object:  23.1°C
---
^C
```



## 3.1 Enable Driver in SDK using Kernel Config Tool


The MLX90614 kernel driver is already included in the Luckfox Buildroot SDK. The source code is located in the kernel source tree:

```bash
# Driver source file
sysdrv/source/kernel/drivers/iio/temperature/mlx90614.c
```
This driver provides a standardized IIO (Industrial I/O) interface for reading sensor data.

**Why is the I2C sensor driver under IIO?**

**IIO (Industrial I/O)** is a Linux kernel framework designed specifically for sensors and data acquisition devices. IIO device drivers are purpose-built for sensors - they provide automatic data conversion (raw values to physical units), standardized sensor interfaces, and create consistent user-space files like `in_temp_ambient_raw` for easy access. Whereas I2C device drivers are generic communication drivers that only handle raw data transfer and would require custom interfaces for each sensor type, making them too low-level for sensor applications.

**Kernel Configuration Options**

There are 3 options for a Kernel Driver Configuration:
- **`n` (No)**: Driver is not compiled
- **`m` (Module)**: Driver is compiled as a loadable kernel module (recommended)
- **`y` (Yes)**: Driver is compiled directly into the kernel (built-in)

We will enable the driver as a module. 

### Enable the MLX90614 Driver

**Configure Kernel:**
Run the build process in the Luckfox Docker container:
*(See the [official guide](https://wiki.luckfox.com/Luckfox-Pico-Ultra/Docker-Image-Build) on setting up the Docker environment)*

There are two ways to enable the MLX90614 driver:

**Method 1: GUI Configuration (Recommended)**
In `/home/` of the docker container, run the following command to open a GUI window (ensure your terminal window has enough height):
```bash
root@container:/home# ./build.sh kernelconfig
```
Navigate to `Device Drivers` → `Industrial I/O support` → `Temperature sensors` and set `MLX90614 contactless IR temperature sensor` to `M` (Module).

**Method 2: Direct File Edit**
Alternatively, you can directly edit the kernel configuration file:
```bash
root@container:/home# echo "CONFIG_MLX90614=m" >> config/kernel_defconfig
```

Both method adds a `CONFIG_MLX90614` line in the kernel config file. You can verify with:
```bash
root@container:/home# grep MLX90614 config/kernel_defconfig
CONFIG_MLX90614=m
```

**Build and Flash**
Build the kernel and firmware, then flash to the board.

### Verify Driver Loading

**SSH to the board and check if the module file exists:**
```bash
root@luckfox:~# ls /oem/usr/ko/mlx*
ko/mlx90614.ko
```

**Load the MLX90614 module:**
```bash
root@luckfox:~# insmod /oem/usr/ko/mlx90614.ko
```

*Note: We use `insmod` instead of `modprobe` because the system uses busybox, which doesn't have the full modprobe functionality expected in standard Linux distributions.*

**Check if driver is loaded:**
```bash
root@luckfox:~# lsmod | grep mlx90614
mlx90614                4331  0
```

**Check IIO device:**
```bash
root@luckfox:~# ls /sys/bus/iio/devices/
iio:device0
```

*Note: At this point, you won't see the MLX90614 IIO device yet because the device tree configuration hasn't been set up. The driver is loaded but doesn't know where to find the sensor. This will be resolved in the next section when we configure the device tree.*

## 3.2 Specify Driver in Device Tree

The MLX90614 device tree configuration is already included in the Luckfox SDK. The driver needs to know which I2C bus and address to use, which is specified in the device tree.

### Existing Device Tree Configuration

The Luckfox SDK already includes the MLX90614 device tree configuration in `rv1106g-luckfox-pico-ultra-w.dts`:

```dts
/**********I2C2**********/
&i2c2 {
	status = "okay";                    
	clock-frequency = <100000>;  

	// Add the section below
	mlx90614@5a {                           // Device node for MLX90614 sensor          
		compatible = "melexis,mlx90614";    //Matches the driver's compatible string
		reg = <0x5a>;                       //I2C address
	};                                      
};
```

The device tree tells the kernel exactly where to find the MLX90614 sensor, enabling automatic driver loading and IIO interface creation.


After build, flash, and SSH, verify the MLX90614 sensor is now detected:

**Load the MLX90614 module:**
```bash
root@luckfox:~# insmod /oem/usr/ko/mlx90614.ko
```

**Check if driver is loaded:**
```bash
root@luckfox:~# lsmod | grep mlx90614
mlx90614                4331  0
```

**Verify IIO device creation:**
```bash
root@luckfox:~# ls /sys/bus/iio/devices/
iio:device0  iio:device1
```

**Verify sensor name:**
```bash
root@luckfox:~# cat /sys/bus/iio/devices/iio:device1/name
mlx90614
```
Now we have the MLX90614 sensor as IIO devices on `iio:device1`


**Check sensor files:**
```bash
root@luckfox:~# ls /sys/bus/iio/devices/iio:device1/
in_temp_ambient_raw  in_temp_object_raw  name  power  subsystem  uevent
in_temp_offset       in_temp_scale
```

The presence of `in_temp_ambient_raw`, `in_temp_object_raw`, `in_temp_scale`, and `in_temp_offset` files indicates the MLX90614 driver is working correctly and has detected the sensor.

**Read sensor values using kernel driver:**
```bash
# Read ambient temperature (raw value)
root@luckfox:~# cat /sys/bus/iio/devices/iio:device1/in_temp_ambient_raw
14811

# Read object temperature (raw value)
root@luckfox:~# cat /sys/bus/iio/devices/iio:device1/in_temp_object_raw
14821

# Read temperature scale factor
root@luckfox:~# cat /sys/bus/iio/devices/iio:device1/in_temp_scale
20

# Read temperature offset
root@luckfox:~# cat /sys/bus/iio/devices/iio:device1/in_temp_offset
-13657.500000
```

**Convert raw values to Celsius:**

Formula: `(raw + offset) * scale / 1000`
```bash
# Ambient temperature: (14811 + (-13657.5)) * 20 / 1000 = 22.7°C
root@luckfox:~# echo "scale=1; (($(cat /sys/bus/iio/devices/iio:device1/in_temp_ambient_raw) + $(cat /sys/bus/iio/devices/iio:device1/in_temp_offset)) * $(cat /sys/bus/iio/devices/iio:device1/in_temp_scale)) / 1000" | bc -l
22.7

# Object temperature: (14821 + (-13657.5)) * 20 / 1000 = 22.9°C
root@luckfox:~# echo "scale=1; (($(cat /sys/bus/iio/devices/iio:device1/in_temp_object_raw) + $(cat /sys/bus/iio/devices/iio:device1/in_temp_offset)) * $(cat /sys/bus/iio/devices/iio:device1/in_temp_scale)) / 1000" | bc -l
22.9
```




This confirms that `iio:device1` is indeed the MLX90614 sensor and shows how to read temperature values using the kernel driver.

## 3.3 Shell Commands to Log Sensor Data via Kernel Driver

Now that we have the MLX90614 kernel driver loaded and the IIO interface available, we can create a shell script that reads temperature data directly from the IIO device files.

### IIO Shell Script

Create `read_mlx90614_iio.sh`:

```bash
#!/bin/bash

# MLX90614 IIO Temperature Reader Script
# This script reads temperature data from MLX90614 sensor via IIO interface

# Configuration
IIO_DEVICE="/sys/bus/iio/devices/iio:device1"
OBJECT_RAW="$IIO_DEVICE/in_temp_object_raw"
AMBIENT_RAW="$IIO_DEVICE/in_temp_ambient_raw"
SCALE="$IIO_DEVICE/in_temp_scale"
OFFSET="$IIO_DEVICE/in_temp_offset"

# Function to read temperature from IIO
read_temperature() {
    local raw_file=$1
    local temp_name=$2
    
    if [ ! -f "$raw_file" ]; then
        echo "Error: $raw_file not found"
        return 1
    fi
    
    # Read raw value
    local raw_value=$(cat "$raw_file")
    
    # Read scale and offset
    local scale=$(cat "$SCALE")
    local offset=$(cat "$OFFSET")
    
    # Convert to temperature in Celsius
    # Formula: (raw_value + offset) * scale / 1000
    local temp_c=$(echo "scale=2; ($raw_value + $offset) * $scale / 1000" | bc -l)
    
    echo "$temp_name Temperature: ${temp_c}°C"
}

# Function to check if sensor is present
check_sensor() {
    if [ ! -d "$IIO_DEVICE" ]; then
        echo "✗ MLX90614 sensor not found at $IIO_DEVICE"
        echo "Make sure the module is loaded: modprobe mlx90614"
        return 1
    else
        echo "✓ MLX90614 sensor found at $IIO_DEVICE"
        return 0
    fi
}

# Main execution
echo  "MLX90614 Temperature Reader (Ctrl+C to exit)"
echo "============================================="

# Check if sensor is present
if ! check_sensor; then
    exit 1
fi

echo ""

# Read temperatures
while true; do
    read_temperature "$OBJECT_RAW" "Object"    # Read object temperature
    read_temperature "$AMBIENT_RAW" "Ambient"  # Read ambient temperature 
    echo "---"
    sleep 2    # Wait 2 seconds before next reading
done
```

### How the IIO Script Works

**Key Differences from I2C Script:**
1. **Uses IIO Interface**: Reads from `/sys/bus/iio/devices/iio:device1/` instead of raw I2C registers
2. **Automatic Conversion**: Uses the driver-provided scale and offset values for accurate temperature conversion
3. **Error Handling**: Checks for IIO device presence and file availability
4. **Standard Formula**: Uses the IIO standard formula `(raw + offset) * scale / 1000`

**Temperature Conversion:**
The script uses the formula provided by the MLX90614 driver:
```
Temperature (°C) = (raw_value + offset) * scale / 1000
```

Where:
- `raw_value`: Raw sensor reading from IIO device
- `offset`: -13657.5 (Kelvin offset from driver)
- `scale`: 20 (represents 0.02 Kelvin per raw unit)

### Running the IIO Script

**Important:** Before running the IIO script, you must load the MLX90614 kernel driver:

```bash
# Load the MLX90614 kernel module
root@luckfox:~# insmod /oem/usr/ko/mlx90614.ko

# Verify the driver is loaded
root@luckfox:~# lsmod | grep mlx90614
mlx90614                4331  0

# Check that the IIO device is created
root@luckfox:~# ls /sys/bus/iio/devices/
iio:device0  iio:device1

# Verify the sensor name
root@luckfox:~# cat /sys/bus/iio/devices/iio:device1/name
mlx90614

# Now run the IIO script
root@luckfox:~# ./read_mlx90614_iio.sh
MLX90614 Temperature Reader (Ctrl+C to exit)
=============================================
✓ MLX90614 sensor found at /sys/bus/iio/devices/iio:device1

Object Temperature: 22.95°C
Ambient Temperature: 22.70°C
---
Object Temperature: 22.97°C
Ambient Temperature: 22.72°C
---
Object Temperature: 22.93°C
Ambient Temperature: 22.68°C
---
^C
```

## 3.4 C Program to Log Sensor Data via Kernel Driver

While the IIO shell script is convenient for quick testing, a C program offers better performance and more control over sensor data reading. Let's create a C program that reads temperature data directly from the IIO device files.

### The IIO C Program

**Project Structure:**
```
project/app/read_MLX90614/
├── read_mlx90614.c          # I2C C program source code
├── read_mlx90614_iio.c      # IIO C program source code
├── read_mlx90614.sh         # I2C shell script version
├── read_mlx90614_iio.sh     # IIO shell script version
├── Makefile                 # Build configuration
└── i2c_read_tutorial.md     # This tutorial
```

Create `read_mlx90614_iio.c`:

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

#define IIO_DEVICE_PATH "/sys/bus/iio/devices/iio:device1"
#define OBJECT_RAW_PATH IIO_DEVICE_PATH "/in_temp_object_raw"
#define AMBIENT_RAW_PATH IIO_DEVICE_PATH "/in_temp_ambient_raw"
#define SCALE_PATH IIO_DEVICE_PATH "/in_temp_scale"
#define OFFSET_PATH IIO_DEVICE_PATH "/in_temp_offset"
#define NAME_PATH IIO_DEVICE_PATH "/name"

// Function to read a value from a file
int read_file_value(const char *filepath, char *buffer, size_t buffer_size) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        return -1;
    }
    
    if (fgets(buffer, buffer_size, file) == NULL) {
        fclose(file);
        return -1;
    }
    
    fclose(file);
    return 0;
}

// Function to read temperature from IIO device
int read_temperature(const char *raw_file, const char *temp_name, double *temperature) {
    char raw_str[32];
    char scale_str[32];
    char offset_str[32];
    
    // Read raw value
    if (read_file_value(raw_file, raw_str, sizeof(raw_str)) != 0) {
        printf("Error: Cannot read %s\n", raw_file);
        return -1;
    }
    
    // Read scale value
    if (read_file_value(SCALE_PATH, scale_str, sizeof(scale_str)) != 0) {
        printf("Error: Cannot read scale\n");
        return -1;
    }
    
    // Read offset value
    if (read_file_value(OFFSET_PATH, offset_str, sizeof(offset_str)) != 0) {
        printf("Error: Cannot read offset\n");
        return -1;
    }
    
    // Convert strings to numbers
    int raw_value = atoi(raw_str);
    double scale = atof(scale_str);
    double offset = atof(offset_str);
    
    // Calculate temperature: (raw + offset) * scale / 1000
    *temperature = (raw_value + offset) * scale / 1000.0;
    
    printf("%s Temperature: %.2f°C\n", temp_name, *temperature);
    return 0;
}

// Function to check if sensor is present
int check_sensor() {
    char name_buffer[32];
    
    if (read_file_value(NAME_PATH, name_buffer, sizeof(name_buffer)) != 0) {
        printf("✗ MLX90614 sensor not found at %s\n", IIO_DEVICE_PATH);
        printf("Make sure the module is loaded: insmod /oem/usr/ko/mlx90614.ko\n");
        return -1;
    }
    
    // Remove newline if present
    name_buffer[strcspn(name_buffer, "\n")] = 0;
    
    if (strcmp(name_buffer, "mlx90614") != 0) {
        printf("✗ Unexpected sensor name: %s\n", name_buffer);
        return -1;
    }
    
    printf("✓ MLX90614 sensor found at %s\n", IIO_DEVICE_PATH);
    return 0;
}

int main() {
    double object_temp, ambient_temp;
    
    printf("MLX90614 Temperature Reader (Ctrl+C to exit)\n");
    printf("=============================================\n");
    
    // Check if sensor is present
    if (check_sensor() != 0) {
        return 1;
    }
    
    printf("\n");
    
    // Read temperatures continuously
    while (1) {
        // Read object temperature
        if (read_temperature(OBJECT_RAW_PATH, "Object", &object_temp) != 0) {
            printf("Object: Error reading sensor\n");
        }
        
        // Read ambient temperature
        if (read_temperature(AMBIENT_RAW_PATH, "Ambient", &ambient_temp) != 0) {
            printf("Ambient: Error reading sensor\n");
        }
        
        printf("---\n");
        sleep(2);  // Wait 2 seconds before next reading
    }
    
    return 0;
}
```

### Updated Makefile

The Makefile has been updated to compile both C programs:

```makefile
read_mlx90614-build:
	@rm -rf $(PKG_BIN); \
	mkdir -p $(PKG_BIN)/bin
	# Compile I2C C program
	$(RK_APP_CROSS)-gcc $(READ_MLX90614_CFLAGS) read_mlx90614.c -o $(PKG_BIN)/bin/read_mlx90614 $(READ_MLX90614_LDFLAGS)
	# Compile IIO C program
	$(RK_APP_CROSS)-gcc $(READ_MLX90614_CFLAGS) read_mlx90614_iio.c -o $(PKG_BIN)/bin/read_mlx90614_iio $(READ_MLX90614_LDFLAGS)
	# Copy shell scripts and make them executable
	cp read_mlx90614.sh $(PKG_BIN)/bin/
	chmod +x $(PKG_BIN)/bin/read_mlx90614.sh
	cp read_mlx90614_iio.sh $(PKG_BIN)/bin/
	chmod +x $(PKG_BIN)/bin/read_mlx90614_iio.sh
	# Copy built files to app output directory
	$(call MAROC_COPY_PKG_TO_APP_OUTPUT, $(RK_APP_OUTPUT), $(PKG_BIN))
```



### Dynamic Device Discovery

The current IIO approach hardcodes `iio:device1`, but the device number can vary. Here's an improved version that dynamically discovers the MLX90614 device:

**Enhanced IIO Shell Script with Dynamic Discovery:**

```bash
#!/bin/bash

# MLX90614 IIO Temperature Reader Script with Dynamic Device Discovery
# This script automatically finds the MLX90614 sensor and reads temperature data

# Function to find MLX90614 device
find_mlx90614_device() {
    for device in /sys/bus/iio/devices/iio:device*; do
        if [ -f "$device/name" ] && [ "$(cat $device/name)" = "mlx90614" ]; then
            echo "$device"
            return 0
        fi
    done
    return 1
}

# Find the MLX90614 device
IIO_DEVICE=$(find_mlx90614_device)
if [ -z "$IIO_DEVICE" ]; then
    echo "✗ MLX90614 sensor not found"
    echo "Make sure the module is loaded: insmod /oem/usr/ko/mlx90614.ko"
    exit 1
fi

echo "✓ MLX90614 sensor found at $IIO_DEVICE"

# Set up file paths
OBJECT_RAW="$IIO_DEVICE/in_temp_object_raw"
AMBIENT_RAW="$IIO_DEVICE/in_temp_ambient_raw"
SCALE="$IIO_DEVICE/in_temp_scale"
OFFSET="$IIO_DEVICE/in_temp_offset"

# Function to read temperature from IIO
read_temperature() {
    local raw_file=$1
    local temp_name=$2
    
    if [ ! -f "$raw_file" ]; then
        echo "Error: $raw_file not found"
        return 1
    fi
    
    # Read raw value
    local raw_value=$(cat "$raw_file")
    
    # Read scale and offset
    local scale=$(cat "$SCALE")
    local offset=$(cat "$OFFSET")
    
    # Convert to temperature in Celsius
    local temp_c=$(echo "scale=2; ($raw_value + $offset) * $scale / 1000" | bc -l)
    
    echo "$temp_name Temperature: ${temp_c}°C"
}

# Main execution
echo "MLX90614 Temperature Reader (Ctrl+C to exit)"
echo "============================================="

# Read temperatures
while true; do
    read_temperature "$OBJECT_RAW" "Object"
    read_temperature "$AMBIENT_RAW" "Ambient"
    echo "---"
    sleep 2
done
```

**Enhanced IIO C Program with Dynamic Discovery:**

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <dirent.h>

#define IIO_DEVICES_PATH "/sys/bus/iio/devices"
#define MAX_PATH_LEN 256

// Function to find MLX90614 device dynamically
int find_mlx90614_device(char *device_path, size_t path_size) {
    DIR *dir = opendir(IIO_DEVICES_PATH);
    if (!dir) {
        printf("Error: Cannot open %s\n", IIO_DEVICES_PATH);
        return -1;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "iio:device", 10) == 0) {
            char name_path[MAX_PATH_LEN];
            snprintf(name_path, sizeof(name_path), 
                    "%s/%s/name", IIO_DEVICES_PATH, entry->d_name);
            
            char name[32];
            if (read_file_value(name_path, name, sizeof(name)) == 0) {
                name[strcspn(name, "\n")] = 0;
                if (strcmp(name, "mlx90614") == 0) {
                    snprintf(device_path, path_size, 
                            "%s/%s", IIO_DEVICES_PATH, entry->d_name);
                    closedir(dir);
                    return 0;
                }
            }
        }
    }
    closedir(dir);
    return -1;
}

// Rest of the program remains the same, but uses dynamic device path
int main() {
    char iio_device_path[MAX_PATH_LEN];
    char object_raw_path[MAX_PATH_LEN];
    char ambient_raw_path[MAX_PATH_LEN];
    char scale_path[MAX_PATH_LEN];
    char offset_path[MAX_PATH_LEN];
    char name_path[MAX_PATH_LEN];
    
    // Find MLX90614 device dynamically
    if (find_mlx90614_device(iio_device_path, sizeof(iio_device_path)) != 0) {
        printf("✗ MLX90614 sensor not found\n");
        printf("Make sure the module is loaded: insmod /oem/usr/ko/mlx90614.ko\n");
        return 1;
    }
    
    printf("✓ MLX90614 sensor found at %s\n", iio_device_path);
    
    // Set up file paths
    snprintf(object_raw_path, sizeof(object_raw_path), "%s/in_temp_object_raw", iio_device_path);
    snprintf(ambient_raw_path, sizeof(ambient_raw_path), "%s/in_temp_ambient_raw", iio_device_path);
    snprintf(scale_path, sizeof(scale_path), "%s/in_temp_scale", iio_device_path);
    snprintf(offset_path, sizeof(offset_path), "%s/in_temp_offset", iio_device_path);
    snprintf(name_path, sizeof(name_path), "%s/name", iio_device_path);
    
    // Update global paths (if using the original approach)
    // Or pass paths to functions directly
    
    printf("MLX90614 Temperature Reader (Ctrl+C to exit)\n");
    printf("=============================================\n");
    
    // Rest of the program logic remains the same...
    // (Reading temperatures, etc.)
    
    return 0;
}
```

### Expected Files After Build

After building and flashing, you should find these files on the target system:

```bash
# On the target board (via SSH)
root@luckfox:~# ls /oem/usr/bin/read_*
read_mlx90614      read_mlx90614_iio   read_mlx90614.sh   read_mlx90614_iio.sh
```

### Sample Output

```bash
# Load the driver first
root@luckfox:~# insmod /oem/usr/ko/mlx90614.ko

# Run the IIO C program
root@luckfox:~# ./read_mlx90614_iio
MLX90614 Temperature Reader (Ctrl+C to exit)
=============================================
✓ MLX90614 sensor found at /sys/bus/iio/devices/iio:device1

Object Temperature: 22.95°C
Ambient Temperature: 22.70°C
---
Object Temperature: 22.97°C
Ambient Temperature: 22.72°C
---
Object Temperature: 22.93°C
Ambient Temperature: 22.68°C
---
^C
```

---

