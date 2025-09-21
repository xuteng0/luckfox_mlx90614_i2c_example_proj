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
