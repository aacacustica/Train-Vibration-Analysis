#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

#include "measurement_types.h"

class SDLogger {
 public:
  explicit SDLogger(SPIClass &spi);

  bool begin();

  bool isReady() const;

  bool appendAverage(
      const AverageResult &result
  );

  bool appendVibration(
      const VibrationReport &report,
      uint32_t totalOverruns
  );

  void listRoot();

 private:
  bool ensureCSVHeader(
      const char *path,
      const char *header
  );

  void listDirectory(
      const char *dirname,
      uint8_t levels
  );

  SPIClass &spi_;
  bool ready_ = false;
};