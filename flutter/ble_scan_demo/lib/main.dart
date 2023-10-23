import 'dart:convert';
import 'package:flutter/material.dart';
import 'package:flutter_blue/flutter_blue.dart';

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
            },
          ),
          Expanded(
            child: ListView.builder(
              itemCount: devices.length,
              itemBuilder: (context, index) {
                return ListTile(
                  title: Text(devices[index].name == "" ? "(unknown device)" : devices[index].name),
                  trailing: ElevatedButton(
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
                  ),
                  tileColor: devices[index].name == "Spark Analyzer" ? Colors.white : Colors.grey,
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

  @override
  void initState() {
    super.initState();
    discoverServices();
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

    if (targetCharacteristic != null) {
      const interval = Duration(seconds: 1);
      Stream.periodic(interval).listen((_) {
        sendBluetoothData();
      });
    }
  }

  sendBluetoothData() async {
    if (targetCharacteristic == null) return;
    Map<String, dynamic> dataToSend = {
      'output': isOutputOn,
      'voltage': selectedVoltage
    };
    String jsonData = jsonEncode(dataToSend);
    final List<int> data = utf8.encode(jsonData);
    await targetCharacteristic!.write(data);
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
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text("Toggle Output: "),
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
          Row(
            mainAxisAlignment: MainAxisAlignment.center,
            children: [
              Text("Select Voltage: "),
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
            ],
          ),
          ElevatedButton(
            onPressed: () {
              sendBluetoothData();
            },
            child: Text('Send Data'),
          ),
        ],
      ),
    );
  }
}
