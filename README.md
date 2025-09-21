# MLX90614 I2C Temperature Sensor Example

This project demonstrates how to read temperature data from an MLX90614 infrared temperature sensor using I2C communication on the Luckfox Pico platform. It provides both direct I2C register access and kernel driver-based IIO (Industrial I/O) implementations.

## Overview

The MLX90614 is a non-contact infrared temperature sensor that can measure both ambient and object temperatures via I2C interface. This example project is designed to be used as a submodule in the Luckfox Pico SDK and provides multiple approaches to interface with the sensor:

- **Direct I2C Access**: Using i2c-tools and raw I2C register reads
- **Kernel Driver Access**: Using the Linux IIO framework with the MLX90614 kernel driver

## Project Structure

```
luckfox_mlx90614_i2c_example_proj/
├── README.md                    # This file
├── i2c_read_tutorial.md         # Detailed tutorial and setup guide
├── Makefile                     # Build configuration for Luckfox SDK
├── MLX90614-Datasheet-Melexis.pdf  # Official sensor datasheet
├── read_mlx90614.c             # C program using direct I2C access
├── read_mlx90614.sh            # Shell script using i2c-tools
├── read_mlx90614_iio.c         # C program using IIO kernel driver
└── read_mlx90614_iio.sh        # Shell script using IIO interface
```

## Installation as Submodule

This project is designed to be added as a submodule to your Luckfox Pico SDK project under the apps directory.

### Adding as Submodule

```bash
# Navigate to your Luckfox Pico SDK directory
cd /path/to/luckfox-pico

# Add this project as a submodule under project/app/
git submodule add https://github.com/your-username/luckfox_mlx90614_i2c_example_proj.git project/app/read_mlx90614

# Initialize and update the submodule
git submodule update --init --recursive
```

### Directory Structure After Installation

```
luckfox-pico/
├── project/
│   └── app/
│       ├── read_mlx90614/           # This submodule
│       │   ├── README.md
│       │   ├── Makefile
│       │   ├── read_mlx90614.c
│       │   ├── read_mlx90614_iio.c
│       │   └── ...
│       ├── other_app1/
│       └── other_app2/
└── ...
```

## Hardware Setup

### MLX90614 Sensor Specifications
- **I2C Address**: 0x5A
- **Operating Voltage**: 3.3V
- **Interface**: I2C (100kHz)
- **Temperature Range**: -70°C to +380°C (object), -40°C to +125°C (ambient)

### Hardware Connections

Connect the MLX90614 sensor to the Luckfox Pico I2C2 M0 pins:

| MLX90614 Pin | Luckfox Pico Pin | GPIO        |
|--------------|------------------|-------------|
| VCC          | 3.3V            | -           |
| GND          | GND             | -           |
| SDA          | Pin 3           | GPIO1_C7    |
| SCL          | Pin 5           | GPIO1_D0    |

## SDK Configuration

Before building, you need to configure the Luckfox SDK to support I2C and the MLX90614 sensor.

### 1. Enable I2C2 in Device Tree

Add the following to your device tree configuration:

```dts
/**********I2C2**********/
&i2c2 {
    status = "okay";
    clock-frequency = <100000>;
    
    mlx90614@5a {
        compatible = "melexis,mlx90614";
        reg = <0x5a>;
    };
};
```

### 2. Enable Required Software Components

#### Enable i2c-tools (for direct I2C access)
```bash
# In Docker container
./build.sh buildrootconfig
# Navigate to: Target packages → Hardware handling → i2c-tools
# Enable i2c-tools package
```

#### Enable MLX90614 Kernel Driver (for IIO access)
```bash
# In Docker container
./build.sh kernelconfig
# Navigate to: Device Drivers → Industrial I/O support → Temperature sensors
# Set "MLX90614 contactless IR temperature sensor" to 'M' (Module)
```

## Building

### Build in Docker Environment

```bash
# Start Docker container
sudo docker start -ai luckfox
cd /home

# Build the entire project (including this submodule)
./build.sh all

# Flash to device
sudo ./build.sh flash
```

### Build Output

After building, the following executables will be available on the target system in `/oem/usr/bin/`:

- `read_mlx90614` - C program using direct I2C access
- `read_mlx90614.sh` - Shell script using i2c-tools
- `read_mlx90614_iio` - C program using IIO kernel driver
- `read_mlx90614_iio.sh` - Shell script using IIO interface

## Usage

### Method 1: Direct I2C Access

This method directly accesses I2C registers without requiring kernel driver loading.

```bash
# SSH to the Luckfox device
ssh root@<device-ip>

# Verify sensor detection
i2cdetect -y 2

# Run shell script version
./read_mlx90614.sh

# Run C program version
./read_mlx90614
```

**Sample Output:**
```
MLX90614 Temperature Reader (Ctrl+C to exit)
=============================================
Ambient: 23.2°C
Object:  22.8°C
---
Ambient: 23.1°C
Object:  23.0°C
---
```

### Method 2: IIO Kernel Driver Access

This method uses the Linux IIO framework with the MLX90614 kernel driver.

```bash
# SSH to the Luckfox device
ssh root@<device-ip>

# Load the MLX90614 kernel module
insmod /oem/usr/ko/mlx90614.ko

# Verify driver loading
lsmod | grep mlx90614

# Run shell script version
./read_mlx90614_iio.sh

# Run C program version
./read_mlx90614_iio
```

**Sample Output:**
```
MLX90614 Temperature Reader (Ctrl+C to exit)
=============================================
✓ MLX90614 sensor found at /sys/bus/iio/devices/iio:device1

Object Temperature: 22.95°C
Ambient Temperature: 22.70°C
---
Object Temperature: 22.97°C
Ambient Temperature: 22.72°C
---
```

## Implementation Details

### Direct I2C Access Implementation

The direct I2C implementation (`read_mlx90614.c`, `read_mlx90614.sh`) uses:
- **i2c-tools**: Command-line utilities for I2C communication
- **SMBus Protocol**: Standard I2C communication protocol
- **Manual Conversion**: Raw register values converted using the formula: `Temperature (°C) = (raw_value × 0.02) - 273.15`

**Key Registers:**
- `0x06` - Ambient Temperature
- `0x07` - Object Temperature
- `0x0E` - Sensor ID

### IIO Kernel Driver Implementation

The IIO implementation (`read_mlx90614_iio.c`, `read_mlx90614_iio.sh`) uses:
- **MLX90614 Kernel Driver**: Linux IIO framework driver
- **Standardized Interface**: Files in `/sys/bus/iio/devices/iio:deviceX/`
- **Automatic Conversion**: Driver provides scale and offset for accurate temperature calculation

**Key IIO Files:**
- `in_temp_ambient_raw` - Raw ambient temperature value
- `in_temp_object_raw` - Raw object temperature value
- `in_temp_scale` - Scale factor (typically 20)
- `in_temp_offset` - Offset value (typically -13657.5)

## Troubleshooting

### Common Issues

1. **Sensor Not Detected**
   ```bash
   # Check I2C bus is available
   ls /dev/i2c*
   
   # Scan for I2C devices
   i2cdetect -y 2
   ```

2. **Module Loading Failed**
   ```bash
   # Check if module file exists
   ls /oem/usr/ko/mlx90614.ko
   
   # Load module manually
   insmod /oem/usr/ko/mlx90614.ko
   ```

3. **IIO Device Not Found**
   ```bash
   # Check IIO devices
   ls /sys/bus/iio/devices/
   
   # Verify sensor name
   cat /sys/bus/iio/devices/iio:device*/name
   ```

### Expected Values

- **Ambient Temperature**: Should be close to room temperature (20-25°C)
- **Object Temperature**: Varies based on the object being measured
- **Temperature Accuracy**: ±0.5°C (typical)

## Documentation

For detailed setup instructions, hardware configuration, and comprehensive explanations, see:
- [`i2c_read_tutorial.md`](i2c_read_tutorial.md) - Complete tutorial with step-by-step setup
- [`MLX90614-Datasheet-Melexis.pdf`](MLX90614-Datasheet-Melexis.pdf) - Official sensor datasheet

## Contributing

This project serves as an example implementation. When contributing:

1. Follow the existing code style and structure
2. Test both I2C and IIO implementations
3. Update documentation for any new features
4. Ensure compatibility with Luckfox Pico SDK

## License

This project is provided as an example for educational and development purposes. Please refer to the Luckfox SDK license for usage terms.

## References

- [Luckfox Pico SDK Documentation](https://wiki.luckfox.com/)
- [MLX90614 Sensor Documentation](https://www.melexis.com/en/product/MLX90614/Digital-Plug-Play-Infrared-Thermometer-TO-Can)
- [Linux I2C Subsystem](https://www.kernel.org/doc/Documentation/i2c/)
- [Linux IIO Framework](https://www.kernel.org/doc/html/latest/driver-api/iio/index.html)