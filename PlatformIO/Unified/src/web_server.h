/**
 * web_server.h - Web Server with REST API
 * 
 * Async web server for web interface and REST API endpoints
 * Provides real-time data, control, and configuration
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

class WebServer {
public:
  WebServer();
  void begin();
  void update();
  
private:
  AsyncWebServer* server;
  bool initialized;
  
  // Setup routes
  void setupRoutes();
  void setupRestAPI();
  void setupWebSocket();
  
  // Route handlers
  void handleRoot(AsyncWebServerRequest* request);
  void handleStatus(AsyncWebServerRequest* request);
  void handleVoltage(AsyncWebServerRequest* request);
  void handleCurrent(AsyncWebServerRequest* request);
  void handlePower(AsyncWebServerRequest* request);
  void handleControl(AsyncWebServerRequest* request);
  void handleSettings(AsyncWebServerRequest* request);
  void handleHistory(AsyncWebServerRequest* request);
  void handleReset(AsyncWebServerRequest* request);
  
  // Helper functions
  void sendJsonResponse(AsyncWebServerRequest* request, const String& json);
  void sendErrorResponse(AsyncWebServerRequest* request, int code, const char* message);
};

#endif // WEB_SERVER_H