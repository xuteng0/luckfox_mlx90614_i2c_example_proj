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
