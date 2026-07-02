#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

static const uint8_t REG_DEVID       = 0x00;                                                                                                // Identificador de dispositivo.
static const uint8_t REG_BW_RATE     = 0x2C;                                                                                                // Frecuencia de salida de datos.
static const uint8_t REG_POWER_CTL   = 0x2D;                                                                                                // Estado de alimentación y el modo de medida.
static const uint8_t REG_INT_SOURCE  = 0x30;                                                                                                // Indica qué eventos internos se han producido.
static const uint8_t REG_DATA_FORMAT = 0x31;                                                                                                // Configura rango de medida resolución completa y justificación de los datos.
static const uint8_t REG_DATAX0      = 0x32;                                                                                                // Dirección del primer byte de los datos de aceleración. A partir de ahí se encuentran seis bytes:



bool initADXL345() {                                                                                                                        
  delay(100);                                                                                                                                // Estabilización después del arranque

  digitalWrite(ADXL_CS, HIGH);                                                                                                               // Deseleccionar para permitir transferencia de datos en vez de entenderlos como comando/señal
  digitalWrite(SD_CS, HIGH);    

  const uint8_t deviceId = readRegister(REG_DEVID);                                                                                          // Lectura de registro REG_DEVID = 0x00
  Serial.printf("ADXL345 DEVID: 0x%02X\n", deviceId);                                                                                        // Si no se obtiene ADXL345_EXPECTED_ID = 0xE5 se detiene la inicialización y se lanza error

  if (deviceId != ADXL345_EXPECTED_ID) {
    Serial.printf(
        "ERROR: se esperaba DEVID 0x%02X y se obtuvo 0x%02X\n",
        ADXL345_EXPECTED_ID,
        deviceId
    );
    return false;
  }

  // Configurar antes de entrar en modo de medida.
  writeRegister(REG_POWER_CTL, 0x00);                                                                                                         // Configuración antes de medir -> REG_POWER_CTL = 0x00 -> sensor fuera de modo de medida para configurar formato y frecuencia antes de tener activada la adquisición
  writeRegister(REG_DATA_FORMAT, ADXL_DATA_FORMAT_VALUE);                                                                                     // REG_DATA_FORMAT = Resolución completa rango +-4g
  writeRegister(REG_BW_RATE, ADXL_BW_RATE_VALUE);                                                                                             // REG_BW_RATE = 100Hz
  writeRegister(REG_POWER_CTL, 0x08);  // Measure = 1.                                                                                        // Ahora sí, REG_POWER_CTL = 0x08 (1) -> empieza medición

  delay(20);                                                                                                                                  // Con 100Hz una muestra tarda aprox 10ms , delay = 20 permite que aparezcan 1/2 muestras

  const uint8_t formatReadback = readRegister(REG_DATA_FORMAT);                                                                               // Se vuelven a leer los registros configurados. Por ahora solo para impresión. Esta re-lectura puede permitir comprobación antes de leer muestras.
  const uint8_t rateReadback   = readRegister(REG_BW_RATE);

  Serial.printf(
      "DATA_FORMAT=0x%02X, BW_RATE=0x%02X\n",
      formatReadback,
      rateReadback
  );

  int16_t testX;
  int16_t testY;
  int16_t testZ;

  // Esperar brevemente a que aparezca una muestra nueva.
  const uint32_t timeoutStart = millis();                                                                                                      // Se guarda el tiempo de espera.
  while ((readRegister(REG_INT_SOURCE) & MASK_DATA_READY) == 0) {                                                                              // Se continua esperando mientras no exista una muestra nueva.
    if ((uint32_t)(millis() - timeoutStart) > 100) {                                                                                           // Si pasan mas de 100ms devuelve error. Con F=100Hz en ese tiempo se deberían generar 10 muestras
      Serial.println("ERROR: timeout esperando DATA_READY");
      return false;
    }
    delay(1);                                                                                                                                  // Evita consultar el registro a máxima velocidad durante inicialización
  }

  if (!readADXLRaw(testX, testY, testZ)) {                                                                                                     // Lectura inicial de prueba, obtiene la primera muestra y enseña valores crudos y convertidos a g
    return false;                                                                                                                              // g = testX/testY/testZ * G_PER_LSB
  }                                                                                                                                            // Para comprobar validez de resultados, con el sensor quieto el vector total debería rondar 1g distribuido entre los ejes según orientación.

  Serial.printf(
      "Lectura inicial: X=%d Y=%d Z=%d LSB | %.3f %.3f %.3f g\n",
      testX,
      testY,
      testZ,
      testX * G_PER_LSB,
      testY * G_PER_LSB,
      testZ * G_PER_LSB
  );

  return true;
}

bool readADXLRaw(int16_t &xOut, int16_t &yOut, int16_t &zOut) {                                                                                 //
  uint8_t data[6];                                                                                                                              // Lectura de 6 bytes consecutivos
  readRegisters(REG_DATAX0, sizeof(data), data);                                                                                                // Lee registros
                                                                                                                                                // El sensor devuelve el byte bajo primero. Si data[0] = 0x34 y data[1] = 0x12 entonces data[1] << 8 = 0x1200 , 0x1200 | 0x34=0x1234
  xOut = static_cast<int16_t>(                                                                                                                  // Static_cast<int16_t> interpreta el resultado como entero con signo
      (static_cast<uint16_t>(data[1]) << 8) | data[0]
  );

  yOut = static_cast<int16_t>(
      (static_cast<uint16_t>(data[3]) << 8) | data[2]
  );

  zOut = static_cast<int16_t>(
      (static_cast<uint16_t>(data[5]) << 8) | data[4]
  );

  return true;
}

void writeRegister(uint8_t reg, uint8_t value) {
  digitalWrite(SD_CS, HIGH);

  spiADXL.beginTransaction(
      SPISettings(ADXL_SPI_HZ, MSBFIRST, SPI_MODE3)
  );

  digitalWrite(ADXL_CS, LOW);
  spiADXL.transfer(reg & 0x3F);
  spiADXL.transfer(value);
  digitalWrite(ADXL_CS, HIGH);

  spiADXL.endTransaction();
}

uint8_t readRegister(uint8_t reg) {
  uint8_t value = 0;
  readRegisters(reg, 1, &value);
  return value;
}

void readRegisters(uint8_t startReg, uint8_t numBytes, uint8_t *buffer) {
  uint8_t address = startReg | 0x80;  // Lectura.

  if (numBytes > 1) {
    address |= 0x40;                  // Multibyte.
  }

  digitalWrite(SD_CS, HIGH);

  spiADXL.beginTransaction(
      SPISettings(ADXL_SPI_HZ, MSBFIRST, SPI_MODE3)
  );

  digitalWrite(ADXL_CS, LOW);
  spiADXL.transfer(address);

  for (uint8_t i = 0; i < numBytes; i++) {
    buffer[i] = spiADXL.transfer(0x00);
  }

  digitalWrite(ADXL_CS, HIGH);
  spiADXL.endTransaction();
}
