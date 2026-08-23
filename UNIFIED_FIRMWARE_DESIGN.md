# Spark Analyzer Unified Firmware Design Document

## Overview
This document defines the architecture and features for a unified firmware that replaces the multiple PlatformIO projects (Template, Template_PPS, WebApp, WebApp_PPS) with a single, production-ready firmware for the Spark Analyzer USB-C Power Delivery device.

## Design Goals
- Single firmware for all use cases (PD + PPS + Web Control)
- Automatic detection and optimal mode selection
- Modern provisioning via BLE
- Professional web interface with full features
- Robust security and update mechanisms
- Excellent user experience through LED feedback

## Core Design Decisions

### 1. Power Delivery Mode Detection
**Strategy: Capability-based auto-detection (B)**
- Device reads source capabilities on connection
- Automatically selects PPS mode if source supports it
- Falls back to PD mode if PPS unavailable
- Optimizes for dynamic voltage changes (PPS) when possible
- Remembers mode preference per source in NVS

### 2. LED Status Indication
**Strategy: Hybrid LED patterns (C)**
- **Power On LED**: Hardware-controlled (device powered)
- **Output Enable LED**: Output status (solid = on, off = disabled)
- **Debug LED**: Source capability indication
  - Off: No source connected
  - Solid: PD source detected
  - Fast blink: PPS source detected
  - Slow blink: Source detection in progress
  - Very fast blink: PD renegotiation/restart

### 3. WiFi Provisioning
**Strategy: BLE provisioning (C)**
- Uses existing BLE capability for credential transfer
- No AP mode required (no network switching)
- Modern, elegant user experience
- Fallback to AP mode if BLE fails
- Device name: "Spark Analyzer" (configurable)

### 4. Web Interface Features
**Strategy: Full-featured (C)**
- Real-time voltage/current/power display
- Dual-mode voltage control:
  - PD mode: Preset buttons (5V, 9V, 12V, 15V, 20V)
  - PPS mode: Slider + fine-tuning controls
- Output on/off toggle
- Current limit setting (PPS: 50mA steps, PD: fixed)
- Real-time current/power graphs
- Historical data logging and CSV export
- Multiple source profiles
- Advanced safety settings
- Firmware update via web interface
- Mobile-first responsive design

### 5. Data Storage
**Strategy: LittleFS + Preferences (C)**
- **Preferences (NVS)**: Settings, profiles, WiFi credentials
- **LittleFS**: Historical data logs, CSV exports, firmware images
- Storage policy: Last 24 hours of data with configurable intervals
- 1-second sampling for real-time display
- 1-minute sampling for long-term storage

### 6. PPS Voltage/Current Control
**Strategy: Dual-mode controls (D)**
- PD mode: Quick preset buttons for standard voltages
- PPS mode: Slider for coarse adjustment, fine-tuning buttons
- UI automatically adapts based on detected source
- Show both voltage and current controls simultaneously
- Handle PD renegotiation restarts transparently

### 7. PD Renegotiation Handling
**Strategy: Smart mode preference (C)**
- Default to PPS mode if source supports it (dynamic changes)
- Use PD mode only if PPS unavailable
- Transparent restart handling for PD voltage changes
- LED indication during renegotiation (very fast blink)
- Web app auto-reconnects after restart
- Remember mode preference per source

### 8. Security Strategy
**Strategy: Basic security (B)**
- Simple password protection for web interface
- API requires same authentication
- Optional password protection (default: no password)
- Local network access only (no cloud)
- HTTPS optional (local network use)

### 9. API Support
**Strategy: REST + MQTT (C)**
- **REST API**: HTTP endpoints for all web UI functions
- **MQTT**: Real-time data streaming and home automation
- Perfect for Home Assistant, Node-RED integration
- **API Security**: Basic auth with same password as web UI
- APIs enabled by default with authentication

### 10. Cloud Integration
**Strategy: Local only (A)**
- No cloud integration (privacy-focused, simple)
- Users can set up own remote access (VPN, port forwarding)
- Avoids ongoing cloud infrastructure costs
- Reduces attack surface

### 11. Network Discovery
**Strategy: mDNS enabled by default (A)**
- Automatic discovery as `spark-analyzer.local`
- User-friendly access without IP addresses
- **Hostname**: Hybrid approach (D)
  - Default: `spark-analyzer.local`
  - Auto-suffix if conflict detected (e.g., `spark-analyzer-a1b2.local`)
  - User can customize via web UI

### 12. Programmable Button
**Strategy: Multi-function (D)**
- **Short press**: Toggle power output on/off
- **Long press (3s)**: WiFi reset (clear credentials, trigger BLE provisioning)
- **Very long press (10s)**: Factory reset
- **LED feedback**: Blink patterns to confirm actions
  - 1 blink: Output toggle
  - 3 blinks: WiFi reset
  - 10 blinks: Factory reset

### 13. Factory Reset Protection
**Strategy: Two-stage confirmation (B)**
- Hold button for 10 seconds continuously
- Release within 2-second window to confirm
- LED patterns guide user through process
- Prevents accidental triggers

### 14. Factory Reset Scope
**Strategy: Configurable (C)**
- **Network Reset**: WiFi credentials only (common case)
- **Full Reset**: Settings, WiFi, and all historical data
- Programmable button triggers Network Reset
- Full Reset requires web UI confirmation
- Great for different troubleshooting scenarios

### 15. Safe Mode
**Strategy: Safe mode on boot failure (C)**
- Automatic detection of consecutive boot failures
- Boots in minimal state with basic web interface
- Allows firmware recovery or settings reset
- No user intervention required
- Standard embedded systems practice

### 16. OTA Updates
**Strategy: Advanced OTA (C)**
- Firmware upload via web interface
- Firmware verification before installation
- Rollback support if update fails
- Version checking for compatibility
- Robust and user-friendly

### 17. Update Checking
**Strategy: Manual check button (B)**
- "Check for Updates" button in web UI
- Queries GitHub API for version info
- Shows if newer version available
- User-initiated, no background calls
- Simple and convenient

### 18. Update Notification Display
**Strategy: Smart notification (D)**
- Prominent banner on first visit after check
- Moves to subtle indicator after dismissal
- Always visible but not intrusive
- Excellent user experience balance

## Technical Architecture

### Hardware Components
- **Microcontroller**: ESP32-C3FH4 (40 MHz, 4MB Flash, WiFi + BLE)
- **USB-C Controller**: ON Semiconductor FUSB302MPX
- **Current Sensor**: CC6904SO-10A Hall effect sensor
- **Power Management**: TI TPS62175DQCT DC-DC converter
- **Indicators**: 3x LEDs (Power On, Output Enable, Debug)
- **Controls**: Reset button, Programmable button

### Software Architecture
```
┌─────────────────────────────────────────────────────────┐
│                   Application Layer                      │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │ Web Server   │  │ MQTT Client  │  │ REST API     │ │
│  │ (AsyncWeb)   │  │              │  │              │ │
│  └──────────────┘  └──────────────┘  └──────────────┘ │
├─────────────────────────────────────────────────────────┤
│                   Business Logic                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │ PD/PPS Engine│  │ Data Logger  │  │ Config Mgr   │ │
│  │ (Auto-detect)│  │ (LittleFS)   │  │ (Preferences)│ │
│  └──────────────┘  └──────────────┘  └──────────────┘ │
├─────────────────────────────────────────────────────────┤
│                   Hardware Layer                        │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │ FUSB302 Driver│  │ ADC/Current  │  │ GPIO/LEDs    │ │
│  │ (I2C)         │  │ Sensor       │  │ Control      │ │
│  └──────────────┘  └──────────────┘  └──────────────┘ │
├─────────────────────────────────────────────────────────┤
│                   Connectivity                          │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐ │
│  │ WiFi Manager │  │ BLE Provision│  │ mDNS Service │ │
│  │ (Network)    │  │ (Credentials)│  │ (Discovery)  │ │
│  └──────────────┘  └──────────────┘  └──────────────┘ │
└─────────────────────────────────────────────────────────┘
```

### Key Libraries
- **PD/PPS**: Custom PD_UFP library (FUSB302 driver + protocol)
- **Web Server**: ESPAsyncWebServer
- **WiFi**: WiFiManager for network management
- **BLE**: NimBLE-Arduino for provisioning
- **Storage**: Preferences (NVS) + LittleFS
- **MQTT**: AsyncMqttClient
- **Discovery**: ESPmDNS
- **OTA**: ArduinoOTA + custom web update

### File Structure
```
PlatformIO/Unified/
├── platformio.ini
├── src/
│   ├── main.cpp              # Main application
│   ├── pd_pps_engine.cpp/h   # PD/PPS auto-detection & control
│   ├── web_server.cpp/h      # Web interface & REST API
│   ├── mqtt_client.cpp/h     # MQTT integration
│   ├── data_logger.cpp/h     # Data storage & retrieval
│   ├── config_manager.cpp/h  # Settings & preferences
│   ├── ble_provisioning.cpp/h # BLE WiFi provisioning
│   ├── led_controller.cpp/h  # LED status indication
│   ├── button_handler.cpp/h  # Multi-function button
│   ├── ota_manager.cpp/h     # Firmware updates
│   └── safe_mode.cpp/h       # Recovery mechanism
├── data/
│   └── index.html            # Web interface
├── lib/
│   └── PD_UFP/               # USB PD library
│       ├── PD_UFP.cpp/h
│       ├── FUSB302_UFP.cpp/h
│       └── PD_UFP_Protocol.cpp/h
└── include/
    └── config.h              # Configuration constants
```

## Implementation Priority

### Phase 1: Core Infrastructure
1. Set up unified PlatformIO project structure
2. Implement LED controller with status patterns
3. Implement button handler with multi-function support
4. Implement config manager (Preferences)
5. Implement data logger (LittleFS)

### Phase 2: Connectivity
6. Implement BLE provisioning system
7. Implement WiFi manager with mDNS
8. Implement safe mode and factory reset

### Phase 3: Power Delivery
9. Implement PD/PPS auto-detection engine
10. Implement smart mode preference logic
11. Handle PD renegotiation restarts

### Phase 4: Web Interface
12. Implement web server core
13. Implement REST API endpoints
14. Create web interface HTML/CSS/JS
15. Implement real-time data streaming

### Phase 5: Advanced Features
16. Implement MQTT client
17. Implement advanced OTA updates
18. Implement update checking from GitHub
19. Implement historical data display and export

### Phase 6: Testing & Polish
20. Comprehensive testing of all features
21. Security audit and hardening
22. Performance optimization
23. Documentation completion

## Security Considerations
- Default: No password required (local network)
- Optional password protection for web UI and APIs
- Basic authentication for REST/MQTT
- No cloud dependencies (local only)
- Safe mode for recovery
- Firmware verification during OTA updates
- Input validation on all API endpoints

## Performance Targets
- Boot time: < 3 seconds to ready state
- Web page load: < 1 second on local network
- API response: < 100ms for local requests
- Current sampling: 1-second intervals for real-time
- Data logging: 1-minute intervals for historical
- PD negotiation: < 2 seconds for standard profiles
- PPS voltage change: < 500ms (no restart)

## Testing Strategy
- Unit tests for core PD/PPS logic
- Integration tests for web API
- Hardware-in-loop testing with actual PD sources
- Stress testing with continuous operation
- Security testing for API endpoints
- OTA update testing with various scenarios
- Recovery testing (safe mode, factory reset)

## Success Criteria
- Single firmware replaces all existing variants
- Automatic PD/PPS detection works with 95%+ of sources
- BLE provisioning success rate > 90%
- Web interface loads and functions reliably
- All APIs respond within performance targets
- OTA updates succeed > 95% of the time
- Recovery mechanisms work in all failure scenarios
- User testing shows improved satisfaction over current multi-firmware approach

## Future Enhancements (Post-MVP)
- Mobile app for native experience
- Additional protocol support (CAN, Modbus)
- Enhanced graphing and analysis features
- Cloud integration as optional add-on
- Multi-device management interface
- Advanced power profiling and reporting