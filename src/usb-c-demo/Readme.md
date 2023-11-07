# PuTTY for Arduino Telnet Connection Guide

## Downloading and Installing PuTTY

### Download PuTTY:

1. Visit the official PuTTY download page: [PuTTY Download](https://www.putty.org/).
2. Click on the download link for the latest version under "Package files".
3. Choose `putty-<version>-installer.msi` to download the installer (`<version>` is the latest version number).

### Install PuTTY:

1. Find the `.msi` file in your Downloads folder and double-click it.
2. Follow the prompts to install PuTTY. Default settings should be fine.
3. Once installed, PuTTY is ready for use.

## Connecting to the Arduino Device Using PuTTY

### Launch PuTTY:

1. Open the Start menu.
2. Search for "PuTTY" and open the application.

### Configure PuTTY for Telnet:

1. In the "Session" category:
   - Select `Telnet` as the "Connection type:".
   - Enter the IP address of your Arduino in "Host Name (or IP address)" field.
   - Ensure "Port" is set to `23` or your specific Telnet port if different.

2. (Optional) Save the session:
   - Name your session in "Saved Sessions".
   - Click "Save" to save the session settings.

### Open the Telnet Connection:

1. Click "Open" to start the Telnet session.
2. A terminal window will appear if the connection is successful.

### Using Telnet Commands:

- Type the commands in the terminal and press Enter:
  - `?` - Display debug instructions.
  - `V` - Get USB-PD voltage setting.
  - `I` - Get current draw in mA.
  - `C` - Cycle voltage settings.
  - `S` - Toggle current draw streaming.
  - `O` - Toggle output on/off.
  - `R` - Restart device.

### Closing the Connection:

- Close the PuTTY window or type `exit` and Enter to end the session.

## Troubleshooting

- Check that the Arduino is on and connected to the WiFi network.
- Ensure the IP address is correctly entered in PuTTY.
- If connection is refused, verify that the Arduino's Telnet server is active and the port is correct.
- Firewalls or antivirus programs can block Telnet connections. Adjust settings if necessary.

Follow these instructions for a successful Telnet connection to your Arduino using PuTTY on Windows.
