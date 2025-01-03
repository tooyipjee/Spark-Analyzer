# Spark Analyzer Quickstart Guide

Thank you for your purchase! Let's get started.

Your Spark Analyzer ships with the Web App PD firmware. Follow this guide to begin. For more detailed information, refer to the datasheet in the [project repository](link-to-repository).

## Hardware Setup

1. **Connect to a USB-C PD Compatible Wall Wart:** Ensure your wall adapter supports USB-C PD by looking for "USB PD" markings or output specifications above 5V (e.g., 9V, 15V, 20V).  USB-C alone doesn't guarantee PD compatibility. [See an example](link-to-example-image).

2. **Power On:** Turn on your Spark Analyzer. A blinking debug LED (5 times) confirms the firmware is loaded correctly.

## WiFi Connection

1. **Connect to "Spark Analyzer" WiFi:**  Using a WiFi-enabled device (phone or laptop), connect to the "Spark Analyzer" network.  Internet access isn't required for this step.

2. **Configure WiFi:** Open a web browser and navigate to `192.168.4.1` to access WiFiManager. Enter your home or office WiFi credentials.

## Accessing the Web App

### Find Device IP

* **Serial Monitor:** Connect the device to your computer via USB. The IP address will display in your IDE's serial monitor once connected to WiFi.

* **BLE Advertisement:**  Use a BLE scanner app (like NRF Connect) to locate the device broadcasting its IP address.

### Web App Interface

1. **Launch Web App:** Enter the device's IP address into your web browser.

### Web Interface Overview

* **Current Graph:**  Displays real-time current measurements.
* **Select and Set Voltage:** Choose and apply the desired voltage.
* **Output Control:**  Turn the device's power output on or off.

That's it! You're ready to control your Spark Analyzer's USB-C PD functions. For more examples and applications, explore the [project repository](link-to-repository).
