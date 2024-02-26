# Spark Analyzer Firmware Collection

Welcome to the Spark Analyzer Firmware Collection! This repository is a comprehensive suite of firmware scripts specifically designed for the Spark Analyzer, a versatile tool for power management and smart device control. The firmware collection showcases a range of functionalities, from wireless communication and smart home integration to precise power delivery and current monitoring. Each script is crafted to demonstrate and utilize the unique features of the Spark Analyzer, making them ideal for a variety of applications in IoT, smart home, and power management sectors. Below is an overview of the available firmware scripts:

Most of them are Arduino examples and there is one template PlatformIO firmware that you can use to build your projects. Here are the main scripts/examples;
## 1. [Spark Analyzer Firmware with BLE and USB-C PD Control](https://github.com/tooyipjee/Spark-Analyzer/tree/master/src/Arduino/flutter_controller)
- **Purpose**: Focuses on Bluetooth Low Energy (BLE) communication for remote control and monitoring. [Google Play](https://play.google.com/store/apps/details?id=com.elektrothing.SparkAnalyzer&hl=en&gl=US)
- **Key Features**:
  - Manages USB-C Power Delivery (PD) for voltage control.
  - Implements a moving average filter for stable current readings.
  - Utilizes EEPROM for storing voltage settings.
  - LED indicators for operational status.

## 2. [Spark Analyzer Firmware for ESP-NOW Communication](https://github.com/tooyipjee/Spark-Analyzer/tree/master/src/Arduino/esp-now)
- **Purpose**: Provides host and peripheral setup for ESP-NOW communication.
- **Host Script**:
  - Sends voltage and enable/disable commands to peripherals.
- **Peripheral Script**:
  - Receives commands to control a NeoPixel LED strip and manage USB-C PD.
  - Processes structured data from the host.

## 3. [Spark Analyzer Firmware for Matter Integration](https://github.com/tooyipjee/Spark-Analyzer/tree/master/src/Arduino/matter_controller)
- **Purpose**: Integrates with Matter (formerly Project CHIP) for smart home connectivity.
- **Key Features**:
  - Control of an LED via Matter's OnOff cluster.
  - USB-C PD integration for voltage management.
  - Toggle button implementation with debouncing.
  - Serial communication for device status and debugging.

## 4. [Spark Analyzer Template Firmware for Custom Applications](https://github.com/tooyipjee/Spark-Analyzer/tree/master/src/Arduino/template)
- **Purpose**: Serves as a base for developers creating custom applications.
- **Key Features**:
  - Includes USB-C PD management for voltage setting.
  - Real-time ADC reading with a moving average filter.
  - Configurable initial output state and current limit.
  - Serial communication for debugging and status updates.

## 5. [PlatformIO Template Firmware for USB-C PD and Current Monitoring](https://github.com/tooyipjee/Spark-Analyzer/tree/master/src/PlatformIO/SparkAnalyzer)
- **Purpose**: Optimized for use in PlatformIO environment.
- **Key Features**:
  - USB-C PD initialization and management.
  - Real-time current measurement with a moving average filter.
  - Function prototypes for better code organization.
  - Configurable update interval for monitoring.

Each firmware script in this collection highlights different capabilities of the Spark Analyzer, catering to a wide range of applications in power management, smart home systems, and IoT devices.
