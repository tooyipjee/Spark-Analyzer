/**
 * data_logger.h - Data Logging and Storage Manager
 * 
 * Manages historical data logging using LittleFS:
 * - Current, voltage, power measurements
 * - CSV export functionality
 * - Data retention management
 */

#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <Arduino.h>
#include <LittleFS.h>

// Data point structure
struct DataPoint {
  unsigned long timestamp;
  float voltage;
  float current;
  float power;
  bool outputEnabled;
  uint8_t sourceType;
};

class DataLogger {
public:
  DataLogger();
  void begin();
  void update();
  
  // Data logging
  void logData(float voltage, float current, bool outputEnabled);
  void logDataPoint(const DataPoint& point);
  
  // Data retrieval
  DataPoint* getRecentData(uint32_t count);
  DataPoint* getDataInRange(unsigned long startTime, unsigned long endTime);
  uint32_t getDataCount();
  
  // Data export
  String exportToCSV(unsigned long startTime, unsigned long endTime);
  String exportToJSON(unsigned long startTime, unsigned long endTime);
  
  // Data management
  void clearOldData();
  void clearAllData();
  uint32_t getStorageUsage();
  
private:
  bool initialized;
  String dataFilePath;
  uint32_t dataCount;
  unsigned long lastRetentionCheck;
  
  void ensureFileSystem();
  void checkRetention();
  bool writeDataPoint(const DataPoint& point);
  DataPoint readDataPoint(uint32_t index);
};

#endif // DATA_LOGGER_H