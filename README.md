# Spark Analyzer: The Programmable USB-C Power Delivery Analyzer & Power Supply

<p align="center">
<img src="./images/SparkAnalyzer.jpg" width="800" height="444"/>
</p>


**Just received your Spark Analyzer? Check out our quickstart [here](https://www.elektrothing.com/spark-analyzer-quickstart)!**

**Spark Analyzer** is a versatile ESP32-powered device tailored for enhancing project development and debugging processes. It is fully compatible with USB-C Power Delivery (UCPD) and is further enhanced by a Programmable Power Supply (PPS) feature, allowing for precise control and management of power. Its compact design facilitates easy inline integration with existing setups, providing essential insights and control over power delivery.

Equipped with WiFi and BLE, the Spark Analyzer offers wireless connectivity, enabling developers to remotely monitor and adjust voltage levels, log data, and analyze power consumption through a user-friendly smartphone interface. This design prioritizes convenience and efficiency, removing the need for physical buttons and supporting distance operation

Fully funded on Crowdsupply [now](https://www.crowdsupply.com/elektrothing/spark-analyzer).

[![Spark Analyzer](https://img.youtube.com/vi/IWUR4Ur0Dy4/0.jpg)](https://www.youtube.com/watch?v=IWUR4Ur0Dy4)

## Features:

1. **Power Control and Analysis with PPS and PD**: The Spark Analyzer excels in power management with its Programmable Power Supply (PPS) and Power Delivery (PD) capabilities. Precisely control voltage from 3.3V to 21V at 20 mV resolution and set current limits up to 3A with 50mA resolution with PPS or use PD for adjustable voltage output options (5V, 9V, 15V, 20V). Combined with its current sensor, it allows for accurate current draw measurements, essential for understanding power usage in your projects.

2. **Wireless Control**: Untehther your development process with the Spark Analyzer’s wireless capabilities. With both WiFi and BLE, this tool allows remote, app-based control and data logging. Users can adjust settings and monitor data from a distance, offering a more flexible development experience.

3. **Open Source & Programmable**: As an open-source device, the Spark Analyzer offers unmatched versatility. It is fully programmable, allowing you to tailor its functionality to your specific applications. This customization extends to software and hardware, opening up endless possibilities for creative and efficient project development.

4. **Compact Design**: Designed for convenience, the Spark Analyzer's compact and sleek form factor allows for seamless integration into existing setups. Its compatibility with UCPD power sources and inline integration capability minimizes the need for additional equipment or extensive redesign, making it a versatile and user-friendly addition to any development environment.

5. **Safety**: Safety is crucial when working with electricity and is at the heart of Spark Analyzer. An integrated output FET allows for software programmable current limit that switches off the power in high-current scenarios. 
  
## Specifications

#### Power
- **Negotiable Power Delivery**: Options of 5 VDC, 9 VDC, 12 VDC, 15 VDC, 20 VDC; max 5 A (100 W at 20 VDC). It is also PPS compatible up to 21 VDC.
- **USB Type-C Port**: For power delivery and integrated JTAG programming.
- **ON Semiconductor [FUSB302MPX](https://www.onsemi.com/pdf/datasheet/fusb302b-d.pdf)**: Programmable USB Type-C control and USB PD communication.
- **ESD Protection**: On D+/D-/CC1/CC2 pins.
- **Texas Instruments [TPS62175DQCT](https://www.ti.com/product/TPS62175/part-details/TPS62175DQCT)**: 3.3 VDC 0.5 A max output DC-DC Step-Down Converter.
- **Power Output**: 3.5 mm, 2-position terminal block.

#### I/O Configuration
- **GPIOs**: 4 GPIOs (I2C, UART, SPI compatible).
- **Power Pins**: 1x 5 VDC and 1x GND.

#### Microcontroller
- **Model**: [ESP32-C3FH4](https://www.espressif.com/sites/default/files/documentation/esp32-c3_datasheet_en.pdf) with 40 MHz crystal.
- **Wi-Fi**: 802.11b/g/n.
- **Bluetooth**: BLE 4.2.
- **Flash Memory**: 4 MB.

#### Interface
- **3x LED Indicators**
  - Power On
  - Output Enable
  - Programmable LED/Debug
- **Buttons**
  - Reset
  - Programmable Button/Debug

#### Programming
- **Integrated JTAG Controller**: For programming.
- **USB-C**: Built-in USB JTAG programmer (ESP32-C3), compatible with Arduino. Select ESP32-C3 Dev Board.

#### Output
- **Current Output**: [CC6904SO-10A](https://datasheet.lcsc.com/lcsc/2304140030_Cross-chip-CC6904SO-10A_C469389.pdf).
- **Current Sensor**: Hall Effect Current Sensor.
- **Output Enable**: [DMP3017SFG-7 FET](https://www.diodes.com/assets/Datasheets/products_inactive_data/DMP3017SFG.pdf).

  
<p align="center">
<img src="./images/TopDown.jpg" width="600" height="400"/>
</p>


## Spark Analyzer Mobile App 

<p align="center">
<img src="./images/App.jpg" width="400" height="800"/>
</p>

> **Note**: The Spark Analyzer Mobile App is currently in active development. The features described below are subject to change as we continue to improve the app.

The Spark Analyzer Mobile App, currently done for [Android](https://play.google.com/store/apps/details?id=com.elektrothing.SparkAnalyzer&hl=en&gl=US) (iOS support is available via [RemoteXY](https://apps.apple.com/us/app/remotexy/id1168130280), is the perfect companion to your Spark Analyzer device. It simplifies monitoring and controlling power delivery with these key features:

- **Voltage Selector**: Easily choose from different voltage levels (5 VDC, 9 VDC, 15 VDC, or 20 VDC) using a user-friendly selector.
- **Output Toggle**: Turn the power output on or off conveniently with a toggle switch.
- **Current Logging**: Save current draw as a .csv file.
- **Current Limit**: Ability to set a software current trip as a safety feature.
- **Read-time Display**: View current set point, current draw and output enable in real time on your phone. 

### Key Features:

- **Voltage Selector**: The app comes with a user-friendly voltage selector that allows you to easily choose from the different voltage levels supported by the UCPD protocol. Whether you need 5V, 9V, 15V, or 20V, switching between these options is just a tap away.

- **Output Toggle**: Need to turn the power output on or off? The app features a convenient toggle switch that gives you full control over the Spark Analyzer's output, making it simple to manage your device remotely.

- **Current Draw Chart**: Keep an eye on your project's power consumption with the app's real-time current draw chart. This feature provides a graphical representation of the current being drawn, helping you to monitor and optimize your power usage effectively.

### Future Developments:

The Spark Analyzer Mobile App is continuously being improved, with several new features in the pipeline. Upcoming updates will include the ability to export logs for further analysis, set current limits to prevent overcurrent scenarios, and much more.

## Testing for Performance

At the heart of Spark Analyzer's design is a commitment to reliability and performance. To ensure that Spark Analyzer stands up to real-world demands, we subjected it to testing under use conditions.

### Max Current Test: 3A at 9V
Under a load condition of 3A at 9V, Spark Analyzer showcased its robust power delivery capabilities without any hitches. 

<p align="center">
<img src="./images/9V_3A.jpg" width="800" height="450"/>
</p>

### Max Voltage Test: 1.5A at 20V
Even at its maximum voltage output of 20V with a 1.5A load, Spark Analyzer continued to perform seamlessly, demonstrating its versatility and reliability.

<p align="center">
<img src="./images/20V_1.5A.jpg" width="800" height="450"/>
</p>

### Thermal Performance
Safety and efficiency are paramount. We took a thermal image of the board under these testing conditions to ensure that all components remained within safe temperature limits. The results were impressive: the entire board stayed cool, with the warmest component being the ESP32 microcontroller, which only reached around 40°C.

<p align="center">
<img src="./images/Thermal.jpg" width="600" height="800"/>
</p>

These tests underscore Spark Analyzer's commitment to delivering a product that is not only feature-rich but also reliable, safe, and efficient. With Spark Analyzer, you're investing in a device that's been tried and tested to ensure optimal performance under all conditions.


## Package Contents:

- 1x Spark Analyzer
- Additional components (Specify as per your package)

## Manufacturing & Logistics:

Our designs have undergone rigorous testing and verification. We've already produced a pilot batch, and our manufacturing partners are geared up for full-scale production. All Spark Analyzer units will be shipped globally, ensuring timely delivery to our backers.

## Comparison: Spark Analyzer vs PD Micro vs Lab Power Supply

|    | Spark Analyzer               | [PD Micro](https://www.mouser.co.uk/ProductDetail/Crowd-Supply/CS-PDMICRO-01?qs=TuK3vfAjtkXixx0TeJooNQ%3D%3D)               | [TinyPICO V3](https://www.conrad.com/en/p/joy-it-com-zy12pdn-converter-1-pc-s-2475888.html?WT.srch=1&vat=true&utm_source=google&utm_medium=organic&utm_campaign=shopping&srsltid=AfmBOooPUzUXbytdxZNQd-7snqUbn7jtgCwQktmcI8B5WYmZAfE5yNWl4q0#productDownloads)          | [Joy-IT COM ZY12PDN](https://shop.pimoroni.com/products/tinypico-v2?variant=39285089534035)   |
|------------------------------|------------------------------|------------------------|----------------------|----------------------|
| **Microcontroller**          | ESP32-C3                     | ATmega32U4             | ESP32-PICO-D4        | Not Applicable       |
| **WiFi Connectivity**        | Available (802.11b/g/n)      | Not Available          | Available            | Not Available        |
| **Bluetooth Connectivity**   | Available (BLE 4.2)          | Not Available          | Available            | Not Available        |
| **Flash Memory**             | 4 MB                         | Not Specified          | 4 MB                 | Not Specified        |
| **USB-C Power Delivery**     | Supported                    | Supported              | Not Supported        | Supported            |
| **Output Control**           | Adjustable                   | Adjustable             | Not Specified        | Adjustable           |
| **Wireless Control**         | Yes                          | No                     | Yes                  | No                   |
| **Smartphone App Support**   | Yes                          | No                     | No                   | No                   |
| **Power Analysis**           | Accurate                     | Basic                  | Not Specified        | Basic                |
| **Design**                   | Compact and Sleek            | Compact                | Ultra-Small          | Compact              |
| **Open Source Status**       | Yes                          | Yes                    | Yes                  | No                   |
| **Programming Interface**    | USB + JTAG                   | USB                    | USB                  | Not Applicable       |
| **Inline Integration**       | Yes                          | No                     | Not Applicable       | No                   |
| Power Delivery Range         | 5-20 VDC Supported           | 5-20 VDC Supported     | Not Supported        | 5-20 VDC Supported   |
| **Price (USD)**              | $49                          | $28                    | $22                  | $16                  |


## Firmware development

If you want to modify the functionality of the Spark Analyzer or contribute to it's firmware development process, you can follow the steps below.

* clone this repository `git clone https://github.com/tooyipjee/Spark-Analyzer.git`
* install [Visual Studio Code](https://code.visualstudio.com/Docs/setup/setup-overview)
* install the [PlatformIO](https://platformio.org/) plugin in VSCode
* open the project you want to use: [WebApp for Power Delivery](https://github.com/tooyipjee/Spark-Analyzer/tree/master/PlatformIO/WebApp) or [WebApp for Programmable Power Supply](https://github.com/tooyipjee/Spark-Analyzer/tree/master/PlatformIO/WebApp_PPS)
* connect the Spark Analyzer with a usb-C to usb-C cable to your computer
* open the "Monitor" task in PlatformIO
  * you may have to configure the serial port in `platformio.ini` file, e.g.
  ```
    upload_port = /dev/ttyACM0
    monitor_port = /dev/ttyACM0
  ```
* make sure you see the boot messages
* perform the "Build" task to build the firmware
* then perform the "Upload" task to flash the new firmware
* once the flashing is complete, perform "Build Filesystem Image" and "Upload Filesystem Image" tasks

## License

MIT License

Copyright (c) 2023 elektroThing

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
