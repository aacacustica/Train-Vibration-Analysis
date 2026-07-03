#pragma once

#include <Arduino.h>
#include <SPI.h>

#include "measurement_types.h"

class ADXL345Driver {
 public:
  explicit ADXL345Driver(SPIClass &spi);

  // Configura el sensor y comprueba su identidad.
  bool begin();

  /*
    Devuelve true únicamente cuando había una muestra nueva
    y esta ha sido copiada a sample.
  */
  bool readSampleIfReady(AccelSample &sample);

  // Overruns acumulados desde el arranque.
  uint32_t getTotalOverruns() const;

  /*
    Devuelve los overruns del intervalo actual y reinicia
    solamente el contador de intervalo.
  */
  uint32_t takeIntervalOverruns();

 private:
  void writeRegister(
      uint8_t reg,
      uint8_t value
  );

  uint8_t readRegister(uint8_t reg);

  void readRegisters(
      uint8_t startReg,
      uint8_t numBytes,
      uint8_t *buffer
  );

  void readRaw(
      int16_t &x,
      int16_t &y,
      int16_t &z
  );

  SPIClass &spi_;

  bool ready_ = false;

  uint32_t intervalOverruns_ = 0;
  uint32_t totalOverruns_ = 0;
};