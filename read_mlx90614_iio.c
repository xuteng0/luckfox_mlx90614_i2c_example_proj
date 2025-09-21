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
