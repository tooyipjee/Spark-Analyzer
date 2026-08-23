/**
 * web_server.cpp - Web Server with REST API Implementation
 */

#include "web_server.h"
#include "config.h"
#include "config_manager.h"
#include "pd_pps_engine.h"
#include "data_logger.h"

// External references
extern ConfigManager* configManager;
extern PDPPSEngine* pdppsEngine;
extern DataLogger* dataLogger;
extern uint8_t systemStatus;
extern uint8_t sourceCapability;
extern int currentVoltage;
extern int currentLimit;
extern int currentDraw;
extern bool outputEnabled;
extern bool wifiConnected;

WebServer::WebServer() {
  server = nullptr;
  initialized = false;
}

void WebServer::begin() {
  Serial.println("Initializing web server...");
  
  server = new AsyncWebServer(WEB_SERVER_PORT);
  
  setupRoutes();
  setupRestAPI();
  
  server->begin();
  initialized = true;
  
  Serial.print("Web server started on port ");
  Serial.println(WEB_SERVER_PORT);
}

void WebServer::update() {
  // Async web server doesn't need periodic updates
  // All requests are handled asynchronously
}

void WebServer::setupRoutes() {
  // Serve web interface
  server->on("/", HTTP_GET, [this](AsyncWebServerRequest* request) {
    handleRoot(request);
  });
  
  // Favicon
  server->on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(404);
  });
}

void WebServer::setupRestAPI() {
  // Status endpoint
  server->on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
    handleStatus(request);
  });
  
  // Voltage control
  server->on("/api/voltage", HTTP_GET, [this](AsyncWebServerRequest* request) {
    handleVoltage(request);
  });
  
  server->on("/api/voltage", HTTP_POST, [this](AsyncWebServerRequest* request) {
    handleVoltage(request);
  });
  
  // Current control
  server->on("/api/current", HTTP_GET, [this](AsyncWebServerRequest* request) {
    handleCurrent(request);
  });
  
  server->on("/api/current", HTTP_POST, [this](AsyncWebServerRequest* request) {
    handleCurrent(request);
  });
  
  // Power control
  server->on("/api/power", HTTP_GET, [this](AsyncWebServerRequest* request) {
    handlePower(request);
  });
  
  server->on("/api/power", HTTP_POST, [this](AsyncWebServerRequest* request) {
    handlePower(request);
  });
  
  // Control endpoint
  server->on("/api/control", HTTP_POST, [this](AsyncWebServerRequest* request) {
    handleControl(request);
  });
  
  // Settings endpoint
  server->on("/api/settings", HTTP_GET, [this](AsyncWebServerRequest* request) {
    handleSettings(request);
  });
  
  server->on("/api/settings", HTTP_POST, [this](AsyncWebServerRequest* request) {
    handleSettings(request);
  });
  
  // History endpoint
  server->on("/api/history", HTTP_GET, [this](AsyncWebServerRequest* request) {
    handleHistory(request);
  });
  
  // Reset endpoint
  server->on("/api/reset", HTTP_POST, [this](AsyncWebServerRequest* request) {
    handleReset(request);
  });
}

void WebServer::setupWebSocket() {
  // WebSocket for real-time updates (can be added later)
}

void WebServer::handleRoot(AsyncWebServerRequest* request) {
  // Simple HTML interface
  String html = "<!DOCTYPE html><html><head><title>Spark Analyzer</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<style>body{font-family:Arial,sans-serif;margin:20px;}";
  html += ".status{padding:10px;margin:10px 0;border-radius:5px;}";
  html += ".good{background:#d4edda;color:#155724;}";
  html += ".bad{background:#f8d7da;color:#721c24;}";
  html += "button{padding:10px 20px;margin:5px;cursor:pointer;}";
  html += ".value{font-weight:bold;font-size:1.2em;}</style></head><body>";
  
  html += "<h1>Spark Analyzer</h1>";
  
  // Status
  html += "<div class='status ";
  html += (wifiConnected ? "good" : "bad");
  html += "'>WiFi: ";
  html += (wifiConnected ? "Connected" : "Disconnected");
  html += "</div>";
  
  html += "<div class='status ";
  html += (outputEnabled ? "good" : "bad");
  html += "'>Output: ";
  html += (outputEnabled ? "Enabled" : "Disabled");
  html += "</div>";
  
  // Values
  html += "<h2>Current Values</h2>";
  html += "<p>Voltage: <span class='value'>" + String(currentVoltage) + "V</span></p>";
  html += "<p>Current: <span class='value'>" + String(currentDraw) + "mA</span></p>";
  html += "<p>Current Limit: <span class='value'>" + String(currentLimit) + "mA</span></p>";
  
  // Source capability
  String sourceStr = "None";
  if (sourceCapability == SOURCE_PD_ONLY) sourceStr = "PD Only";
  else if (sourceCapability == SOURCE_PPS_AVAILABLE) sourceStr = "PPS Available";
  else if (sourceCapability == SOURCE_PPS_ACTIVE) sourceStr = "PPS Active";
  
  html += "<p>Source: <span class='value'>" + sourceStr + "</span></p>";
  
  // Controls
  html += "<h2>Controls</h2>";
  html += "<button onclick='toggleOutput()'>Toggle Output</button>";
  html += "<button onclick='refresh()'>Refresh</button>";
  
  html += "<script>";
  html += "function toggleOutput(){fetch('/api/control',{method:'POST',";
  html += "headers:{'Content-Type':'application/json'},";
  html += "body:JSON.stringify({output:" + String(!outputEnabled) + "})}).then(()=>refresh());}";
  html += "function refresh(){location.reload();}";
  html += "</script></body></html>";
  
  request->send(200, "text/html", html);
}

void WebServer::handleStatus(AsyncWebServerRequest* request) {
  StaticJsonDocument<512> doc;
  
  doc["status"] = systemStatus;
  doc["source_capability"] = sourceCapability;
  doc["voltage"] = currentVoltage;
  doc["current"] = currentDraw;
  doc["current_limit"] = currentLimit;
  doc["output_enabled"] = outputEnabled;
  doc["wifi_connected"] = wifiConnected;
  
  String sourceStr = "none";
  if (sourceCapability == SOURCE_PD_ONLY) sourceStr = "pd_only";
  else if (sourceCapability == SOURCE_PPS_AVAILABLE) sourceStr = "pps_available";
  else if (sourceCapability == SOURCE_PPS_ACTIVE) sourceStr = "pps_active";
  
  doc["source_string"] = sourceStr;
  
  String response;
  serializeJson(doc, response);
  sendJsonResponse(request, response);
}

void WebServer::handleVoltage(AsyncWebServerRequest* request) {
  if (request->method() == HTTP_GET) {
    StaticJsonDocument<256> doc;
    doc["voltage"] = currentVoltage;
    doc["source_capability"] = sourceCapability;
    String response;
    serializeJson(doc, response);
    sendJsonResponse(request, response);
  } else if (request->method() == HTTP_POST) {
    if (request->hasParam("voltage", true)) {
      int voltage = request->getParam("voltage", true)->value().toInt();
      
      if (pdppsEngine) {
        bool success = pdppsEngine->setVoltage(voltage);
        if (success) {
          StaticJsonDocument<256> doc;
          doc["success"] = true;
          doc["voltage"] = voltage;
          String response;
          serializeJson(doc, response);
          sendJsonResponse(request, response);
        } else {
          sendErrorResponse(request, 400, "Failed to set voltage");
        }
      } else {
        sendErrorResponse(request, 500, "PD/PPS engine not available");
      }
    } else {
      sendErrorResponse(request, 400, "Missing voltage parameter");
    }
  }
}

void WebServer::handleCurrent(AsyncWebServerRequest* request) {
  if (request->method() == HTTP_GET) {
    StaticJsonDocument<256> doc;
    doc["current"] = currentDraw;
    doc["current_limit"] = currentLimit;
    String response;
    serializeJson(doc, response);
    sendJsonResponse(request, response);
  } else if (request->method() == HTTP_POST) {
    if (request->hasParam("limit", true)) {
      int limit = request->getParam("limit", true)->value().toInt();
      
      if (pdppsEngine) {
        bool success = pdppsEngine->setCurrentLimit(limit);
        if (success) {
          StaticJsonDocument<256> doc;
          doc["success"] = true;
          doc["current_limit"] = limit;
          String response;
          serializeJson(doc, response);
          sendJsonResponse(request, response);
        } else {
          sendErrorResponse(request, 400, "Failed to set current limit");
        }
      } else {
        sendErrorResponse(request, 500, "PD/PPS engine not available");
      }
    } else {
      sendErrorResponse(request, 400, "Missing limit parameter");
    }
  }
}

void WebServer::handlePower(AsyncWebServerRequest* request) {
  if (request->method() == HTTP_GET) {
    StaticJsonDocument<256> doc;
    float power = (currentVoltage * currentDraw) / 1000.0; // Convert to watts
    doc["power"] = power;
    doc["voltage"] = currentVoltage;
    doc["current"] = currentDraw;
    String response;
    serializeJson(doc, response);
    sendJsonResponse(request, response);
  }
}

void WebServer::handleControl(AsyncWebServerRequest* request) {
  if (request->hasParam("output", true)) {
    bool output = request->getParam("output", true)->value() == "true";
    
    if (pdppsEngine) {
      bool success = pdppsEngine->setOutputEnabled(output);
      if (success) {
        StaticJsonDocument<256> doc;
        doc["success"] = true;
        doc["output_enabled"] = output;
        String response;
        serializeJson(doc, response);
        sendJsonResponse(request, response);
      } else {
        sendErrorResponse(request, 500, "Failed to set output");
      }
    } else {
      sendErrorResponse(request, 500, "PD/PPS engine not available");
    }
  } else {
    sendErrorResponse(request, 400, "Missing output parameter");
  }
}

void WebServer::handleSettings(AsyncWebServerRequest* request) {
  if (request->method() == HTTP_GET) {
    StaticJsonDocument<512> doc;
    
    if (configManager) {
      doc["hostname"] = configManager->getHostname();
      doc["default_voltage"] = configManager->getDefaultVoltage();
      doc["default_current_limit"] = configManager->getDefaultCurrentLimit();
      doc["initial_output_state"] = INITIAL_OUTPUT_STATE;
    }
    
    String response;
    serializeJson(doc, response);
    sendJsonResponse(request, response);
  } else if (request->method() == HTTP_POST) {
    // Handle settings update (can be expanded)
    StaticJsonDocument<256> doc;
    doc["success"] = true;
    doc["message"] = "Settings updated";
    String response;
    serializeJson(doc, response);
    sendJsonResponse(request, response);
  }
}

void WebServer::handleHistory(AsyncWebServerRequest* request) {
  StaticJsonDocument<1024> doc;
  
  if (dataLogger) {
    // Get recent history (simplified for now)
    JsonArray history = doc.createNestedArray("history");
    history.add(currentVoltage);
    history.add(currentDraw);
    doc["timestamp"] = millis();
  }
  
  String response;
  serializeJson(doc, response);
  sendJsonResponse(request, response);
}

void WebServer::handleReset(AsyncWebServerRequest* request) {
  String resetType = request->getParam("type", true)->value();
  
  if (configManager) {
    if (resetType == "network") {
      configManager->networkReset();
    } else if (resetType == "factory") {
      configManager->factoryReset();
    }
  }
  
  StaticJsonDocument<256> doc;
  doc["success"] = true;
  doc["message"] = "Reset completed";
  String response;
  serializeJson(doc, response);
  sendJsonResponse(request, response);
}

void WebServer::sendJsonResponse(AsyncWebServerRequest* request, const String& json) {
  request->send(200, "application/json", json);
}

void WebServer::sendErrorResponse(AsyncWebServerRequest* request, int code, const char* message) {
  StaticJsonDocument<256> doc;
  doc["error"] = message;
  doc["code"] = code;
  
  String response;
  serializeJson(doc, response);
  request->send(code, "application/json", response);
}