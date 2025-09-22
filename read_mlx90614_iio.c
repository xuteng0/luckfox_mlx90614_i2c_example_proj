#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <dirent.h>

#define IIO_DEVICES_PATH "/sys/bus/iio/devices"
#define MAX_PATH_LEN 256

// Forward declarations
int read_file_value(const char *filepath, char *buffer, size_t buffer_size);
int read_temperature(const char *raw_file, const char *scale_file, const char *offset_file, const char *temp_name, double *temperature);

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

// Function to read a value from a file (used for IIO device files)
// Returns 0 on success, -1 on error
int read_file_value(const char *filepath, char *buffer, size_t buffer_size) {
    FILE *file = fopen(filepath, "r");
    if (file == NULL) {
        return -1;  // File not found or permission denied
    }
    
    if (fgets(buffer, buffer_size, file) == NULL) {
        fclose(file);
        return -1;  // Failed to read from file
    }
    
    fclose(file);
    return 0;  // Success
}

// Function to read temperature from IIO device
// Reads raw value, scale, and offset from IIO device files and converts to Celsius
// Returns 0 on success, -1 on error
int read_temperature(const char *raw_file, const char *scale_file, const char *offset_file, const char *temp_name, double *temperature) {
    char raw_str[32];
    char scale_str[32];
    char offset_str[32];
    
    // Read raw temperature value from IIO device
    if (read_file_value(raw_file, raw_str, sizeof(raw_str)) != 0) {
        printf("Error: Cannot read %s\n", raw_file);
        return -1;
    }
    
    // Read scale factor (converts raw units to Kelvin)
    if (read_file_value(scale_file, scale_str, sizeof(scale_str)) != 0) {
        printf("Error: Cannot read scale\n");
        return -1;
    }
    
    // Read offset value (Kelvin offset from raw value)
    if (read_file_value(offset_file, offset_str, sizeof(offset_str)) != 0) {
        printf("Error: Cannot read offset\n");
        return -1;
    }
    
    // Convert strings to numbers
    int raw_value = atoi(raw_str);
    double scale = atof(scale_str);
    double offset = atof(offset_str);
    
    // Calculate temperature using IIO standard formula: (raw + offset) * scale / 1000
    // This converts from raw sensor units to Celsius
    *temperature = (raw_value + offset) * scale / 1000.0;
    
    printf("%s Temperature: %.2f°C\n", temp_name, *temperature);
    return 0;
}


int main() {
    char iio_device_path[MAX_PATH_LEN];
    char object_raw_path[MAX_PATH_LEN];
    char ambient_raw_path[MAX_PATH_LEN];
    char scale_path[MAX_PATH_LEN];
    char offset_path[MAX_PATH_LEN];
    char name_path[MAX_PATH_LEN];
    double object_temp, ambient_temp;
    
    printf("MLX90614 Temperature Reader (Ctrl+C to exit)\n");
    printf("=============================================\n");
    
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
    
    printf("\n");
    
    // Read temperatures continuously
    while (1) {
        // Read object temperature
        if (read_temperature(object_raw_path, scale_path, offset_path, "Object", &object_temp) != 0) {
            printf("Object: Error reading sensor\n");
        }
        
        // Read ambient temperature
        if (read_temperature(ambient_raw_path, scale_path, offset_path, "Ambient", &ambient_temp) != 0) {
            printf("Ambient: Error reading sensor\n");
        }
        
        printf("---\n");
        sleep(2);  // Wait 2 seconds before next reading
    }
    
    return 0;
}