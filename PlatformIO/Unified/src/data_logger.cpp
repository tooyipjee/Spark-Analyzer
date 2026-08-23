/**
 * data_logger.cpp - Data Logging and Storage Manager Implementation
 */

#include "data_logger.h"
#include "config.h"

DataLogger::DataLogger() {
  initialized = false;
  dataFilePath = "/data_log.bin";
  dataCount = 0;
  lastRetentionCheck = 0;
}

void DataLogger::begin() {
  ensureFileSystem();
  initialized = LittleFS.begin();
  
  if (initialized) {
    Serial.println("LittleFS initialized successfully");
    
    // Count existing data points
    if (LittleFS.exists(dataFilePath)) {
      File dataFile = LittleFS.open(dataFilePath, "r");
      if (dataFile) {
        dataCount = dataFile.size() / sizeof(DataPoint);
        dataFile.close();
        Serial.print("Found ");
        Serial.print(dataCount);
        Serial.println(" existing data points");
      }
    }
  } else {
    Serial.println("LittleFS initialization failed");
  }
}

void DataLogger::update() {
  if (!initialized) return;
  
  // Check retention periodically
  unsigned long currentTime = millis();
  if (currentTime - lastRetentionCheck >= 3600000) { // Check every hour
    lastRetentionCheck = currentTime;
    checkRetention();
  }
}

void DataLogger::logData(float voltage, float current, bool outputEnabled) {
  if (!initialized) return;
  
  DataPoint point;
  point.timestamp = millis();
  point.voltage = voltage;
  point.current = current;
  point.power = voltage * current;
  point.outputEnabled = outputEnabled;
  point.sourceType = 0; // This would be set from system state
  
  logDataPoint(point);
}

void DataLogger::logDataPoint(const DataPoint& point) {
  if (!initialized) return;
  
  // Check if we've reached maximum data points
  if (dataCount >= MAX_DATA_POINTS) {
    // Remove oldest data point (circular buffer approach)
    // For simplicity, we'll just clear old data periodically
    checkRetention();
  }
  
  if (writeDataPoint(point)) {
    dataCount++;
  }
}

DataPoint* DataLogger::getRecentData(uint32_t count) {
  if (!initialized || count == 0) return nullptr;
  
  uint32_t actualCount = min(count, dataCount);
  DataPoint* buffer = new DataPoint[actualCount];
  
  if (!buffer) return nullptr;
  
  // Read the most recent data points
  File dataFile = LittleFS.open(dataFilePath, "r");
  if (!dataFile) {
    delete[] buffer;
    return nullptr;
  }
  
  // Seek to the position of the first requested point
  uint32_t startIndex = dataCount - actualCount;
  dataFile.seek(startIndex * sizeof(DataPoint));
  
  for (uint32_t i = 0; i < actualCount; i++) {
    dataFile.read((uint8_t*)&buffer[i], sizeof(DataPoint));
  }
  
  dataFile.close();
  return buffer;
}

DataPoint* DataLogger::getDataInRange(unsigned long startTime, unsigned long endTime) {
  if (!initialized) return nullptr;
  
  // For simplicity, return all data and let the caller filter
  // In a real implementation, we'd want to be more efficient
  return getRecentData(dataCount);
}

uint32_t DataLogger::getDataCount() {
  return dataCount;
}

String DataLogger::exportToCSV(unsigned long startTime, unsigned long endTime) {
  if (!initialized) return "";
  
  String csv = "timestamp,voltage,current,power,output_enabled,source_type\n";
  
  DataPoint* data = getDataInRange(startTime, endTime);
  if (!data) return csv;
  
  uint32_t count = getDataCount();
  for (uint32_t i = 0; i < count; i++) {
    csv += String(data[i].timestamp) + ",";
    csv += String(data[i].voltage) + ",";
    csv += String(data[i].current) + ",";
    csv += String(data[i].power) + ",";
    csv += String(data[i].outputEnabled) + ",";
    csv += String(data[i].sourceType) + "\n";
  }
  
  delete[] data;
  return csv;
}

String DataLogger::exportToJSON(unsigned long startTime, unsigned long endTime) {
  if (!initialized) return "{}";
  
  String json = "{\"data\":[";
  
  DataPoint* data = getDataInRange(startTime, endTime);
  if (!data) return "{}";
  
  uint32_t count = getDataCount();
  for (uint32_t i = 0; i < count; i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"timestamp\":" + String(data[i].timestamp) + ",";
    json += "\"voltage\":" + String(data[i].voltage) + ",";
    json += "\"current\":" + String(data[i].current) + ",";
    json += "\"power\":" + String(data[i].power) + ",";
    json += "\"output_enabled\":" + String(data[i].outputEnabled) + ",";
    json += "\"source_type\":" + String(data[i].sourceType);
    json += "}";
  }
  
  json += "],\"count\":" + String(count) + "}";
  
  delete[] data;
  return json;
}

void DataLogger::clearOldData() {
  if (!initialized) return;
  
  // Remove data older than retention period
  unsigned long cutoffTime = millis() - (DATA_RETENTION_HOURS * 3600000);
  
  // For simplicity, we'll just clear all data and start fresh
  // In a real implementation, we'd want to selectively remove old data
  clearAllData();
}

void DataLogger::clearAllData() {
  if (!initialized) return;
  
  if (LittleFS.exists(dataFilePath)) {
    LittleFS.remove(dataFilePath);
  }
  
  dataCount = 0;
  Serial.println("All data cleared");
}

uint32_t DataLogger::getStorageUsage() {
  if (!initialized) return 0;
  
  if (LittleFS.exists(dataFilePath)) {
    File dataFile = LittleFS.open(dataFilePath, "r");
    if (dataFile) {
      uint32_t size = dataFile.size();
      dataFile.close();
      return size;
    }
  }
  
  return 0;
}

void DataLogger::ensureFileSystem() {
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed, formatting...");
    LittleFS.format();
    LittleFS.begin();
  }
}

void DataLogger::checkRetention() {
  if (!initialized) return;
  
  // Check if we need to remove old data
  if (dataCount > MAX_DATA_POINTS) {
    clearOldData();
  }
}

bool DataLogger::writeDataPoint(const DataPoint& point) {
  File dataFile = LittleFS.open(dataFilePath, "a");
  if (!dataFile) {
    Serial.println("Failed to open data file for writing");
    return false;
  }
  
  size_t written = dataFile.write((uint8_t*)&point, sizeof(DataPoint));
  dataFile.close();
  
  return written == sizeof(DataPoint);
}

DataPoint DataLogger::readDataPoint(uint32_t index) {
  DataPoint point = {0};
  
  if (!initialized || index >= dataCount) return point;
  
  File dataFile = LittleFS.open(dataFilePath, "r");
  if (!dataFile) return point;
  
  dataFile.seek(index * sizeof(DataPoint));
  dataFile.read((uint8_t*)&point, sizeof(DataPoint));
  dataFile.close();
  
  return point;
}