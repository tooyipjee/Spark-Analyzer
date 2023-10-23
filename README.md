# Spark Analyzer: The Ultimate USB-C Power Delivery Analyzer & Power Supply

<p align="center">
<img src="./images/SparkAnalyzer.jpg" width="800" height="600"/>
</p>

**Spark Analyzer** is an innovative and versatile ESP32-powered USB-C Power Delivery (UCPD) compatible device designed to enhance the development and debugging process for projects and prototypes. This compact and sleek power supply and analyzer can be seamlessly integrated inline with existing setups, providing invaluable insights and control over power delivery.

With Spark Analyzer, developers can easily monitor and manipulate voltage levels, log data, and analyze power consumption, all through a convenient and intuitive interface on their smartphones. The device is equipped with both WiFi and BLE connectivity, enabling wireless control and logging, eliminating the need for clunky physical buttons and allowing for remote operation from a distance.

The sleek design of Spark Analyzer ensures it can be effortlessly attached to any existing UCPD compatible wall wart or power source, without requiring additional stands or tables. Its compact form factor guarantees flexibility and convenience, making voltage control and data logging accessible and hassle-free.

## Features:

- **USB-C Power Delivery Compatibility**: Spark Analyzer is fully compatible with USB-C Power Delivery, ensuring compatibility with a myriad of devices and applications.
  
- **Wireless Control and Logging**: Say goodbye to physical buttons. With built-in WiFi and BLE, control and log data wirelessly using your smartphone.
  
- **Adjustable Voltage Output**: Precisely control voltage levels to suit your project's needs. Switchable to 5V, 9V, 15V, and 20V, 
   
- **Compact and Sleek Design**: Integrate Spark Analyzer effortlessly with your existing setups without the need for additional equipment.
  
- **Inline Integration**: Enhance convenience by integrating Spark Analyzer inline with UCPD compatible power sources.

- **IO Breakout**: Incorporate higher voltage into your existing projects with ease. Spark Analyzer features a separate 3.3V power domain.

- **Power Analyzer Capability**: Measure your project's current draw with precision. Understand your power needs and optimize accordingly.

- **Software Safety Cut-off**: No more worries about excessive current draw. Spark Analyzer's current sensing capability ensures the output FET is switched off during high current scenarios, safeguarding your equipment.

- **Output FET**: Toggle high voltage output on and off with the integrated output FET, providing you with more control over your power delivery.

- **Open Source Development**: Customize and expand Spark Analyzer's capabilities with its open-source nature. Dive deep into its functionalities and tailor it to your specific needs.

## Specifications:

- **Microcontroller**: ESP32-C3
- **WiFi**: 802.11b/g/n
- **Bluetooth**: BLE 4.2
- **FLASH**: 4MB
- **Programming**: Integrated JTAG Controller
- **UCPD**: FUSB302MPX
- **Output**: CC6904SO-10A | Current Sense: Hall Effect Current Sensor
- **Output Enable**: DMP3017SFG-7 | FET
- **Power**: TPS62175DQCT | 500mA 3.3V SMPS
  
<p align="center">
<img src="./images/TopDown.jpg" width="600" height="800"/>
</p>

## Why Choose Spark Analyzer?

- **Advanced Power Delivery**: Spark Analyzer is not just another power supply. It's a state-of-the-art USB-C Power Delivery solution tailored for developers, hobbyists, and professionals alike.

- **Seamless Integration**: Its compact design and inline integration capability mean you can effortlessly incorporate Spark Analyzer into any setup, be it a professional lab or a hobbyist's workbench.

- **Wireless Convenience**: Gone are the days of being tethered by wires. With Spark Analyzer's WiFi and BLE capabilities, you can monitor and control your power delivery from anywhere in the room.

- **Safety First**: With features like software safety cut-off and accurate current measurement, Spark Analyzer ensures that your devices are protected from potential overcurrent scenarios.

- **IO Breakout for Versatility**: Spark Analyzer's IO Breakout feature allows you to incorporate higher voltage into your projects, giving you more flexibility and expanding the range of projects you can work on.

- **Cost-effective**: With all its advanced features and capabilities, Spark Analyzer offers unparalleled value for money. Why invest in multiple devices when Spark Analyzer can do it all?

Spark Analyzer is not just a tool; it's a game-changer. Whether you're developing, debugging, or optimizing, Spark Analyzer ensures you do it efficiently, safely,
## Introducing the Spark Analyzer Mobile App (In Development)

<p align="center">
<img src="./images/App.jpg" width="400" height="800"/>
</p>

> **Note**: The Spark Analyzer Mobile App is currently in active development. The features described below are subject to change as we continue to improve the app.

The Spark Analyzer Mobile App serves as the perfect companion to your Spark Analyzer device, offering an intuitive and feature-rich interface right on your smartphone. The app is designed to simplify the process of monitoring and controlling your power delivery, making it easier than ever to manage your projects.

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

| Feature/Specification | **Spark Analyzer** | **PD Micro** | **Lab Power Supply** |
|-----------------------|--------------------|--------------|----------------------|
| **Microcontroller**   | ESP32-C3           | ATmega32U4   | N/A                  |
| **WiFi**              | 802.11b/g/n        | No           | No                   |
| **Bluetooth**         | BLE 4.2            | No           | No                   |
| **FLASH Memory**      | 4MB                | N/A          | N/A                  |
| **USB-C PD**          | Yes                | Yes          | No                   |
| **Output Control**    | Adjustable         | Adjustable   | Fully Adjustable     |
| **Wireless Control**  | Yes                | No           | No                   |
| **Smartphone App**    | Yes                | No           | No                   |
| **Current Measurement**| Accurate          | Basic        | Highly Accurate      |
| **Design**            | Compact & Sleek    | Compact      | Bulky                |
| **Open Source**       | Yes                | Yes          | Varies               |
| **Programming Interface** | Integrated JTAG | Arduino IDE | N/A                  |
| **Inline Integration**| Yes                | No           | No                   |
| **Price**             | TBD                | $28          | >$199                |

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
