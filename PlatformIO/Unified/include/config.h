/**
 * config.h - Spark Analyzer Unified Firmware Configuration
 * 
 * Hardware pin definitions, constants, and configuration parameters
 * for the Spark Analyzer USB-C Power Delivery device.
 */

#ifndef CONFIG_H
#define CONFIG_H

// Hardware Pin Definitions
#define USB_PD_INT_PIN          10      // FUSB302 interrupt pin
#define OUTPUT_ENABLE_PIN       3       // Output enable control pin  
#define CURRENT_SENSE_PIN       2       // Current sensor ADC pin
#define DEBUG_LED_PIN           8       // Programmable LED/Debug pin
#define PROGRAMMABLE_BUTTON_PIN 9       // Programmable button pin (if available)
#define POWER_ON_LED_PIN        -1      // Power On LED (hardware controlled, -1 if not accessible)

// Firmware Configuration
#define FIRMWARE_VERSION "1.0.0"
#define FIRMWARE_NAME "Spark Analyzer Unified"

// I2C Configuration for FUSB302
#define FUSB302_I2C_SDA         1       // I2C SDA pin
#define FUSB302_I2C_SCL         0       // I2C SCL pin
#define FUSB302_I2C_ADDR        0x22    // FUSB302 I2C address
#define FUSB302_I2C_CLOCK       400000  // I2C clock speed (400kHz)

// ADC Configuration
#define ADC_FILTER_LENGTH       10      // Moving average filter length
#define ADC_SAMPLES_CALIBRATION 30      // Number of samples for zero calibration
#define CURRENT_SENSOR_SCALE    5.6865  // Scale factor for current sensor (mA per ADC unit)

// Power Delivery Configuration
#define DEFAULT_VOLTAGE          5       // Default voltage in volts
#define DEFAULT_CURRENT_LIMIT   0       // Default current limit (0 = no limit)
#define INITIAL_OUTPUT_STATE    false   // Initial output state (off)

// Timing Configuration
#define UPDATE_INTERVAL_MS      100     // Main loop update interval (ms)
#define CURRENT_SAMPLE_INTERVAL_MS 1000 // Current sampling interval (ms)
#define DATA_LOG_INTERVAL_MS    60000   // Data logging interval (ms) - 1 minute
#define WEB_UPDATE_INTERVAL_MS  50      // Web interface update interval (ms)

// LED Timing Patterns (ms)
#define LED_BLINK_SHORT_MS      200     // Short blink duration
#define LED_BLINK_LONG_MS       500     // Long blink duration
#define LED_PATTERN_DELAY_MS    1000    // Delay between pattern repetitions

// Button Timing Configuration
#define BUTTON_DEBOUNCE_MS      50      // Button debounce time
#define SHORT_PRESS_MS          1000    // Short press threshold (output toggle)
#define LONG_PRESS_MS           3000    // Long press threshold (WiFi reset)
#define FACTORY_RESET_PRESS_MS  10000   // Factory reset press duration
#define RESET_CONFIRM_WINDOW_MS 2000    // Time window to release button for confirmation

// WiFi Configuration
#define DEFAULT_HOSTNAME        "spark-analyzer"  // Default mDNS hostname
#define WIFI_TIMEOUT_MS         30000   // WiFi connection timeout
#define AP_MODE_TIMEOUT_MS      300000  // AP mode timeout (5 minutes)

// BLE Provisioning Configuration
#define BLE_DEVICE_NAME         "Spark Analyzer"  // BLE device name
#define BLE_SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"  // BLE service UUID
#define BLE_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"  // BLE characteristic UUID
#define BLE_ADVERTISING_TIMEOUT_MS 120000  // BLE advertising timeout (2 minutes)

// Web Server Configuration
#define WEB_SERVER_PORT         80      // HTTP server port
#define MAX_WEB_SOCKETS         5       // Maximum concurrent WebSocket connections
#define WEB_SOCKET_UPDATE_MS    100     // WebSocket update interval

// MQTT Configuration
#define MQTT_ENABLED             true   // Enable MQTT by default
#define MQTT_DEFAULT_PORT        1883   // Default MQTT port
#define MQTT_KEEP_ALIVE          60     // MQTT keep-alive interval
#define MQTT_QOS                 1      // MQTT QoS level
#define MQTT_BUFFER_SIZE         2048   // MQTT buffer size

// Data Storage Configuration
#define DATA_RETENTION_HOURS     24      // How long to keep historical data (hours)
#define MAX_DATA_POINTS          1440    // Maximum data points (24h @ 1min intervals)
#define LITTLEFS_MAX_FILES       10     // Maximum number of files in LittleFS

// OTA Configuration
#define OTA_ENABLED              true   // Enable OTA updates
#define OTA_PASSWORD             ""     // OTA password (empty = no authentication)
#define OTA_PORT                 3232   // OTA port
#define OTA_START_DELAY_MS       5000   // Delay before OTA starts (ms)

// Safe Mode Configuration
#define BOOT_FAILURE_THRESHOLD   3       // Number of consecutive boot failures to trigger safe mode
#define SAFE_MODE_TIMEOUT_MS     300000  // Safe mode auto-restart timeout (5 minutes)

// Security Configuration
#define DEFAULT_PASSWORD         ""     // Default password (empty = no authentication)
#define MAX_PASSWORD_LENGTH      32     // Maximum password length
#define SESSION_TIMEOUT_MS       3600000 // Session timeout (1 hour)

// Update Configuration
#define GITHUB_REPO              "tooyipjee/Spark-Analyzer"  // GitHub repository for updates
#define UPDATE_CHECK_INTERVAL_HOURS 24   // How often to check for updates (hours)

// Debug Configuration
#define DEBUG_SERIAL             true   // Enable serial debug output
#define DEBUG_LED                true   // Enable LED debug patterns
#define DEBUG_BOOT_TIME_MS       3000   // Boot debug display duration

// MQTT Configuration
#define MQTT_ENABLED true
#define MQTT_DEFAULT_PORT 1883
#define MQTT_KEEP_ALIVE 60
#define MQTT_QOS 1
#define MQTT_UPDATE_INTERVAL_MS 5000

// Feature Flags
#define FEATURE_PD               true   // Enable USB-C PD support
#define FEATURE_PPS              true   // Enable PPS support
#define FEATURE_WIFI             true   // Enable WiFi support
#define FEATURE_BLE              false  // Enable BLE support (disabled due to memory constraints)
#define FEATURE_MQTT             true   // Enable MQTT support
#define FEATURE_WEB_SERVER       true   // Enable web server
#define FEATURE_OTA              true   // Enable OTA updates
#define FEATURE_DATA_LOGGING     true   // Enable data logging
#define FEATURE_SAFE_MODE        true   // Enable safe mode

// Error Codes
#define ERROR_NONE               0
#define ERROR_I2C                1
#define ERROR_PD_INIT            2
#define ERROR_WIFI_CONNECT       3
#define ERROR_FILESYSTEM         4
#define ERROR_OTA                5
#define ERROR_CONFIG             6

// Status Codes
#define STATUS_BOOTING           0
#define STATUS_PROVISIONING      1
#define STATUS_CONNECTING        2
#define STATUS_READY             3
#define STATUS_PD_DETECTING      4
#define STATUS_PD_READY          5
#define STATUS_PPS_READY         6
#define STATUS_ERROR             7
#define STATUS_SAFE_MODE         8

// Source Capability Types
#define SOURCE_NONE              0
#define SOURCE_PD_ONLY           1
#define SOURCE_PPS_AVAILABLE     2
#define SOURCE_PPS_ACTIVE        3

#endif // CONFIG_H