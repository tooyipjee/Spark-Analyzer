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
          ElevatedButton(
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
    // Start a timer to read the characteristic value every 2 seconds
    _pollingTimer = Timer.periodic(Duration(seconds: 2), (timer) {
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
    final file = File('$directoryPath/spark_analyzer_log.txt');
    final timestamp = DateTime.now().toIso8601String();
    await file.writeAsString("$timestamp: $data\n", mode: FileMode.append);  }
}


  sendBluetoothData() async {
    if (targetCharacteristic == null) return;
    Map<String, dynamic> dataToSend = {
      'output': isOutputOn,
      'voltage': selectedVoltage,
      'currentLimit' : currentLimit
    };
    String jsonData = jsonEncode(dataToSend);
    final List<int> data = utf8.encode(jsonData);
    await targetCharacteristic!.write(data);
  }

readBluetoothData() async {
  if (targetCharacteristic == null) return;

  List<int> value = await targetCharacteristic!.read();
  print("Received raw data: $value");

  if (value.isNotEmpty) {
    String decodedValue = utf8.decode(value); // Convert the List<int> to a String

    // Parse JSON
    Map<String, dynamic> parsedData = jsonDecode(decodedValue);
    setState(() {
      receivedData = decodedValue;
      voltage = parsedData['Voltage'].toString() ?? "";
      current = parsedData['Current'].toString() ?? "";
      outputEN = parsedData['OutputEN'] ?? false;
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
    body: Column(
      mainAxisAlignment: MainAxisAlignment.center,
      crossAxisAlignment: CrossAxisAlignment.center,
      children: [
        // Control section
        Padding(
          padding: const EdgeInsets.all(8.0),
          child: Column(
            children: [
              Text("Control", style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
              SizedBox(height: 10),
              Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: [
                  // Left column of Control
                  Expanded(
                    child: Column(
                      children: [
                        Text("Select Voltage:"),
                        DropdownButton<String>(
                          value: selectedVoltage,
                          items: ['5', '9', '15', '20'].map((String value) {
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
                        SizedBox(height: 10),
                        Text("Toggle Output:"),
                        Switch(
                          value: isOutputOn,
                          onChanged: (value) {
                            setState(() {
                              isOutputOn = value;
                            });
                          },
                        ),
                        SizedBox(height: 10),
                        Text("Current Limit (mA):"),
                        Container(
                          width: 100,  // Fixed width for appearance
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
                  // Right column of Control
                  Expanded(
                    child: Center(
                      child: ElevatedButton(
                        onPressed: () {
                          sendBluetoothData();
                        },
                        child: Text('Send Data'),
                      ),
                    ),
                  ),
                ],
              ),
            ],
          ),
        ),
        SizedBox(height: 20),
        // Spark Analyzer section
        Text("Spark Analyzer", style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
        Text("Voltage: $voltage"),
        Text("Current: $current"),
        Text("Output EN: ${outputEN ? "Enabled" : "Disabled"}"),
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
  );
}
}