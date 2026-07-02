#pragma once

#include <Arduino.h>
#include <SPI.h>

#include "measurement_types.h"

class ADXL345Driver {
 public:
  explicit ADXL345Driver(SPIClass &spi);

  bool begin();

  // Devuelve true solamente cuando se ha leído una muestra nueva.
  bool readSampleIfReady(AccelSample &sample);

  uint32_t getTotalOverruns() const;

  // Devuelve los overruns del intervalo y pone el contador a cero.
  uint32_t takeIntervalOverruns();

 private:
  void writeRegister(uint8_t reg, uint8_t value);

  uint8_t readRegister(uint8_t reg);

  void readRegisters(
      uint8_t startReg,
      uint8_t numBytes,
      uint8_t *buffer
  );

  bool readRaw(
      int16_t &x,
      int16_t &y,
      int16_t &z
  );

  SPIClass &spi_;

  uint32_t intervalOverruns_ = 0;
  uint32_t totalOverruns_ = 0;
};