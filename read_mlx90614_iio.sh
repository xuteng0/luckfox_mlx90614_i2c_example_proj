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
