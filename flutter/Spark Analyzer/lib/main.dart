import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:flutter_blue/flutter_blue.dart';
import 'dart:async';
import 'dart:math' as math;
import 'dart:io';
import 'package:file_picker/file_picker.dart';
import 'package:path_provider/path_provider.dart';

// Add the following variables to store parsed data
String voltage = "";
String current = "";
String currentLimit = "0";
String currentLimitReceived = "0";


bool outputEN = false;
void main() {
  runApp(MyApp());
}

class MyApp extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      home: ScanPage(),
    );
  }
}

class ScanPage extends StatefulWidget {
  @override
  _ScanPageState createState() => _ScanPageState();
}


class _ScanPageState extends State<ScanPage> {
  FlutterBlue flutterBlue = FlutterBlue.instance;
  List<BluetoothDevice> devices = [];
  bool isScanning = false; 

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text('Spark Analyzer'),
      ),
      body: Column(
        children: [
          Padding(
            padding: const EdgeInsets.all(8.0), // Adds padding around the button
            child: ElevatedButton(
              style: ElevatedButton.styleFrom(
                padding: EdgeInsets.symmetric(horizontal: 30, vertical: 15), // Adjust the padding
                textStyle: TextStyle(fontSize: 20), // Set text style here
                shape: RoundedRectangleBorder(
                  borderRadius: BorderRadius.circular(8), // Optional: add rounded corners
                ),
              ),
              child: Text('Start Scanning'),
              onPressed: () async {
                setState(() {
                  isScanning = true; // <-- Start scanning
                });
                devices = [];
                await flutterBlue.startScan(timeout: Duration(seconds: 4));
                flutterBlue.scanResults.listen((List<ScanResult> results) {
                  for (ScanResult result in results) {
                    var device = result.device;
                    if (!devices.contains(device)) {
                      setState(() {
                        devices.add(device);
                      });
                    }
                  }
                });
                await flutterBlue.stopScan();
                setState(() {
                  isScanning = false; // <-- Stop scanning
                });
              },
            ),
          ),
          if (isScanning) // <-- Display loading wheel if scanning
            CircularProgressIndicator(),
          Expanded(
            child: ListView.builder(
              itemCount: devices.length,
              itemBuilder: (context, index) {
                bool isSparkAnalyzer = devices[index].name == "Spark Analyzer";

                return ListTile(
                  title: Text(devices[index].name.isEmpty ? "(unknown device)" : devices[index].name),
                  trailing: isSparkAnalyzer 
                    ? ElevatedButton(
                        child: Text('Connect'),
                        onPressed: () async {
                          await devices[index].connect();
                          Navigator.push(
                            context,
                            MaterialPageRoute(
                              builder: (context) => ControlPage(device: devices[index]),
                            ),
                          );
                        },
                      )
                    : null, // No button for non-Spark Analyzer devices
                  tileColor: isSparkAnalyzer ? Colors.white : Colors.grey[300],
                );
              },
            ),
          ),
        ],
      ),
    );
  }


}


class ControlPage extends StatefulWidget {
  final BluetoothDevice device;

  ControlPage({required this.device});

  @override
  _ControlPageState createState() => _ControlPageState();
}

class _ControlPageState extends State<ControlPage> {
  bool isOutputOn = false;
  String selectedVoltage = "5";
  BluetoothCharacteristic? targetCharacteristic;
  String receivedData = "No data received.";
  StreamSubscription? deviceConnection;
  String? chosenDirectoryPath;
  Timer? _pollingTimer;
  bool _isLogging = false;
  bool isSendingData = false;

 @override
void initState() {
  super.initState();
  discoverServices();

  // Listen to device connection state changes
  deviceConnection = widget.device.state.listen((state) {
    if (state == BluetoothDeviceState.disconnected) {
      // Device got disconnected. Go back to scan page.
      Navigator.of(context).pop(); // pop current page from stack
      // Optionally disconnect from the device (to ensure clean disconnection)
      widget.device.disconnect();
    }
  });
    // Start a timer to read the characteristic value every 2 seconds TODO
    _pollingTimer = Timer.periodic(Duration(milliseconds: 500), (timer) {
      if (targetCharacteristic != null) {
        readBluetoothData();
      }
    });
  }
@override
void dispose() {
  // Dispose the device connection listener
  deviceConnection?.cancel();
    // Dispose the timer when the widget is disposed to avoid memory leaks
    _pollingTimer?.cancel();
    super.dispose();
  }

discoverServices() async {
  List<BluetoothService> services = await widget.device.discoverServices();
  services.forEach((service) {
    if (service.uuid.toString() == "4fafc201-1fb5-459e-8fcc-c5c9c331914b") {
      service.characteristics.forEach((characteristic) {
        if (characteristic.uuid.toString() == "beb5483e-36e1-4688-b7f5-ea07361b26a8") {
          targetCharacteristic = characteristic;
        }
      });
    }
  });

}

void logDataToFile(String data) async {
  if (_isLogging) {  // Check if logging is enabled
    String directoryPath = chosenDirectoryPath ?? (await getApplicationDocumentsDirectory()).path;
    final file = File('$directoryPath/spark_analyzer_log.csv');
    
    // Ensure the file exists and has a CSV header if it's new
    bool fileExists = await file.exists();
    if (!fileExists) {
      await file.writeAsString("Timestamp,Voltage,Current,OutputEN\n");
    }
    
    // Parse JSON data
    Map<String, dynamic> parsedData = jsonDecode(data);
    String csvData = [
      DateTime.now().toIso8601String(), // Timestamp
      parsedData['Voltage'].toString(), // Voltage
      parsedData['Current'].toString(), // Current
      parsedData['OutputEN'] ? "Enabled" : "Disabled", // Output Enable Status
      parsedData['currentLimit'].toString(), // Current Limit
    ].join(",") + "\n"; // Join with commas and append newline
    
    // Write the CSV data to the file
    await file.writeAsString(csvData, mode: FileMode.append);
  }
}


sendBluetoothData() async {
  if (targetCharacteristic == null || isSendingData) {
    print('Characteristic is not set or data is currently being sent.');
    return;
  }

  isSendingData = true; // Set flag to indicate sending is in progress

  try {
    Map<String, dynamic> dataToSend = {
      'output': isOutputOn,
      'voltage': selectedVoltage,
      'currentLimit': currentLimit
    };
    String jsonData = jsonEncode(dataToSend);
    final List<int> data = utf8.encode(jsonData);

    await targetCharacteristic!.write(data, withoutResponse: false);
    print('Data sent: $jsonData');
  } catch (e) {
    print('Error sending data: $e');
  } finally {
    isSendingData = false; // Clear flag regardless of the result
  }
}



readBluetoothData() async {
  if (targetCharacteristic == null) return;

  List<int> value = await targetCharacteristic!.read();
  print("Received raw data: $value");

  if (value.isNotEmpty) {
    String decodedValue = utf8.decode(value); // Convert the List<int> to a String

    // Parse JSON
    Map<String, dynamic> parsedData = jsonDecode(decodedValue);
    // Check if all expected keys are present
    bool isValidData = parsedData.containsKey('Voltage') &&
                        parsedData.containsKey('Current') &&
                        parsedData.containsKey('OutputEN') &&
                        parsedData.containsKey('currentLimit');
    // If any key is missing, consider the data packet as incomplete/invalid and ignore it
    if (!isValidData) {
      print('Incomplete data packet received, ignoring.');
      return;
    }
    setState(() {
      receivedData = decodedValue;
      voltage = parsedData['Voltage'].toString() ?? "";
      current = parsedData['Current'].toString() ?? "";
      outputEN = parsedData['OutputEN'] ?? false;
      currentLimitReceived = parsedData['currentLimit'].toString() ?? "0";  // Read the currentLimit from JSON

      logDataToFile(decodedValue);

    });
  }
}


@override
Widget build(BuildContext context) {
  return Scaffold(
    appBar: AppBar(
      title: Text('Control Panel'),
    ),
    body: SingleChildScrollView( // 1. Use SingleChildScrollView
      child: Center( // 3. Center the primary content
        child: ConstrainedBox(
          constraints: BoxConstraints(maxWidth: 600),  // This is to ensure that content doesn't spread too wide on tablets or wide screens
          child: Padding( // 4. Add some padding
            padding: const EdgeInsets.all(16.0),
            child: Column(
              mainAxisAlignment: MainAxisAlignment.center,
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                // Inside the Control section
                Card(
                  elevation: 5,
                  child: Padding(
                    padding: const EdgeInsets.all(16.0),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.stretch,
                      children: [
                        Text("Control", style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                        SizedBox(height: 16),
                        Row(
                          children: [
                            // Left column of Control
                            Expanded(
                              child: Column(
                                crossAxisAlignment: CrossAxisAlignment.start,
                                children: [
                                  Text("Select Voltage:"),
                                  DropdownButton<String>(
                                    value: selectedVoltage,
                                    items: ['5', '9', '12', '15', '20'].map((String value) {
                                      return DropdownMenuItem<String>(
                                        value: value,
                                        child: Text(value),
                                      );
                                    }).toList(),
                                    onChanged: (newValue) {
                                      setState(() {
                                        selectedVoltage = newValue!;
                                      });
                                    },
                                  ),
                                  SizedBox(height: 16),
                                  Text("Toggle Output:"),
                                  Switch(
                                    value: isOutputOn,
                                    onChanged: (value) {
                                      setState(() {
                                        isOutputOn = value;
                                      });
                                    },
                                  ),
                                ],
                              ),
                            ),
                            // Right column of Control
                            Expanded(
                              child: Column(
                                mainAxisSize: MainAxisSize.min,
                                children: [
                                  Text('Send to Spark Analyzer'),
                                  SizedBox(height: 8), // Adds space between the text and the icon button
                                  Container(
                                    decoration: BoxDecoration(
                                      color: Theme.of(context).primaryColor, // Default blue color
                                      borderRadius: BorderRadius.circular(8), // Rounded corners
                                    ),
                                    child: IconButton(
                                      iconSize: 24, // Adjust the icon size if needed
                                      icon: Icon(Icons.send, color: Colors.white), // White icon for contrast
                                      onPressed: () {
                                        sendBluetoothData();
                                      },
                                      tooltip: 'Send to Spark Analyzer', // Tooltip text on long press
                                    ),
                                  ),
                                  SizedBox(height: 16), // This maintains the spacing between elements
                                  // Current Limit (mA) entry
                                  Text("Current Limit (mA):"),
                                  Container(
                                    width: 100, // Fixed width for appearance
                                    child: TextField(
                                      keyboardType: TextInputType.number, // Only numbers
                                      onChanged: (String value) {
                                        setState(() {
                                          currentLimit = value;
                                        });
                                      },
                                      decoration: InputDecoration(
                                        hintText: "e.g., 1000",
                                        border: OutlineInputBorder(),
                                      ),
                                    ),
                                  ),
                                ],
                              ),
                            ),
                          ],
                        ),
                      ],
                    ),
                  ),
                ),
                SizedBox(height: 5),
                // Inside the Spark Analyzer section
                Card(
                  elevation: 5,
                  child: Padding(
                    padding: const EdgeInsets.all(16.0),
                    child: Column(
                      crossAxisAlignment: CrossAxisAlignment.stretch,
                      children: [
                        Text("Data from Spark Analyzer", style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                        SizedBox(height: 16),
                        Table(
                          columnWidths: const {
                            0: FlexColumnWidth(),
                            1: FlexColumnWidth(),
                          },
                          border: TableBorder.all(),
                          children: [
                            TableRow(children: [
                              Padding(padding: const EdgeInsets.all(8.0), child: Text("Voltage")),
                              Padding(padding: const EdgeInsets.all(8.0), child: Text(voltage)),
                            ]),
                            TableRow(children: [
                              Padding(padding: const EdgeInsets.all(8.0), child: Text("Current")),
                              Padding(padding: const EdgeInsets.all(8.0), child: Text(current)),
                            ]),
                            TableRow(children: [
                              Padding(padding: const EdgeInsets.all(8.0), child: Text("Output EN")),
                              Padding(padding: const EdgeInsets.all(8.0), child: Text(outputEN ? "Enabled" : "Disabled")),
                            ]),
                            TableRow(children: [
                              Padding(padding: const EdgeInsets.all(8.0), child: Text("Current Limit Set")),
                              Padding(padding: const EdgeInsets.all(8.0), child: Text(currentLimitReceived)),
                            ]),
                          ],
                        ),
                        SizedBox(height: 20),
                        ElevatedButton(
                            child: Text('Choose Save Directory'),
                            onPressed: () async {
                              String? directoryPath = await FilePicker.platform.getDirectoryPath();
                              if (directoryPath != null) {
                                  setState(() {
                                      chosenDirectoryPath = directoryPath;
                                  });
                              }
                            },
                        ),
                        Text("Current Save Directory: ${chosenDirectoryPath ?? "Default App Directory"}"),
                        // Added Logging button
                        ElevatedButton(
                          onPressed: () {
                            setState(() {
                              _isLogging = !_isLogging;  // Toggle logging state
                            });
                          },
                          child: Text(_isLogging ? "Stop Logging" : "Start Logging"),
                        ),
                      ],
                    ),
                  ),
                ),
              ],
            ),
          ),
        ),
      ),
    ),
  );
}

}