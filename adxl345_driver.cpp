
#include <Arduino.h>
#include <SPI.h>

constexpr uint8_t REG_DEVID       = 0x00;                                                                                                // Identificador de dispositivo.
constexpr uint8_t REG_BW_RATE     = 0x2C;                                                                                                // Frecuencia de salida de datos.
constexpr uint8_t REG_POWER_CTL   = 0x2D;                                                                                                // Estado de alimentación y el modo de medida.
constexpr uint8_t REG_INT_SOURCE  = 0x30;                                                                                                // Indica qué eventos internos se han producido.
constexpr uint8_t REG_DATA_FORMAT = 0x31;                                                                                                // Configura rango de medida resolución completa y justificación de los datos.
constexpr uint8_t REG_DATAX0      = 0x32;                                                                                                // Dirección del primer byte de los datos de aceleración. A partir de ahí se encuentran seis bytes:

constexpr uint8_t ADXL345_EXPECTED_ID = 0xE5;

constexpr uint8_t MASK_DATA_READY = 0x80;
constexpr uint8_t MASK_OVERRUN = 0x01;

constexpr uint8_t POWER_CTL_MEASURE = 0x08;

constexpr uint32_t DATA_READY_TIMEOUT_MS = 100UL;

// =========================
// Namespace
// =========================

ADXL345Driver::ADXL345Driver(
    SPIClass &spi
)
    : spi_(spi) {
}

bool ADXL345Driver::begin() {
  ready_ = false;
  intervalOverruns_ = 0;
  totalOverruns_ = 0;

  pinMode(Config::ADXL_CS, OUTPUT);
  digitalWrite(Config::ADXL_CS, HIGH);

  delay(100);

  const uint8_t deviceId =
      readRegister(REG_DEVID);

  Serial.printf(
      "ADXL345 DEVID: 0x%02X\n",
      static_cast<unsigned int>(deviceId)
  );

  if (deviceId != ADXL345_EXPECTED_ID) {
    Serial.printf(
        "ERROR: se esperaba DEVID 0x%02X"
        " y se obtuvo 0x%02X\n",
        static_cast<unsigned int>(
            ADXL345_EXPECTED_ID
        ),
        static_cast<unsigned int>(
            deviceId
        )
    );

    return false;
  }

  // Configurar el sensor fuera del modo de medida.
  writeRegister(
      REG_POWER_CTL,
      0x00
  );

  writeRegister(
      REG_DATA_FORMAT,
      Config::ADXL_DATA_FORMAT_VALUE
  );

  writeRegister(
      REG_BW_RATE,
      Config::ADXL_BW_RATE_VALUE
  );

  // Activar modo de medida.
  writeRegister(
      REG_POWER_CTL,
      POWER_CTL_MEASURE
  );

  delay(20);

  const uint8_t formatReadback =
      readRegister(REG_DATA_FORMAT);

  const uint8_t rateReadback =
      readRegister(REG_BW_RATE);

  Serial.printf(
      "DATA_FORMAT=0x%02X, BW_RATE=0x%02X\n",
      static_cast<unsigned int>(
          formatReadback
      ),
      static_cast<unsigned int>(
          rateReadback
      )
  );

  /*
    En el código original estos registros se imprimían,
    pero no se comprobaban. Aquí sí se valida que la
    configuración escrita coincide con la leída.
  */
  if (
      formatReadback !=
          Config::ADXL_DATA_FORMAT_VALUE ||
      rateReadback !=
          Config::ADXL_BW_RATE_VALUE
  ) {
    Serial.println(
        "ERROR: la configuracion escrita"
        " no coincide con la leida"
    );

    return false;
  }

  // Esperar una primera muestra.
  const uint32_t timeoutStart = millis();

  while (
      (
          readRegister(REG_INT_SOURCE) &
          MASK_DATA_READY
      ) == 0
  ) {
    if (
        static_cast<uint32_t>(
            millis() - timeoutStart
        ) > DATA_READY_TIMEOUT_MS
    ) {
      Serial.println(
          "ERROR: timeout esperando DATA_READY"
      );

      return false;
    }

    delay(1);
  }

  int16_t testX = 0;
  int16_t testY = 0;
  int16_t testZ = 0;

  readRaw(
      testX,
      testY,
      testZ
  );

  Serial.printf(
      "Lectura inicial:"
      " X=%d Y=%d Z=%d LSB"
      " | %.3f %.3f %.3f g\n",
      testX,
      testY,
      testZ,
      testX * Config::G_PER_LSB,
      testY * Config::G_PER_LSB,
      testZ * Config::G_PER_LSB
  );

  ready_ = true;
  return true;
}

bool ADXL345Driver::readSampleIfReady(
    AccelSample &sample
) {
  if (!ready_) {
    return false;
  }

  const uint8_t interruptSource =
      readRegister(REG_INT_SOURCE);

  if (
      (
          interruptSource &
          MASK_OVERRUN
      ) != 0
  ) {
    intervalOverruns_++;
    totalOverruns_++;
  }

  if (
      (
          interruptSource &
          MASK_DATA_READY
      ) == 0
  ) {
    return false;
  }

  sample.timestampUs = micros();

  readRaw(
      sample.x,
      sample.y,
      sample.z
  );

  return true;
}

uint32_t
ADXL345Driver::getTotalOverruns() const {
  return totalOverruns_;
}

uint32_t
ADXL345Driver::takeIntervalOverruns() {
  const uint32_t value =
      intervalOverruns_;

  intervalOverruns_ = 0;

  return value;
}

void ADXL345Driver::writeRegister(uint8_t reg, uint8_t value) {

  spi_.beginTransaction(
      SPISettings(Config::ADXL_SPI_HZ, MSBFIRST, SPI_MODE3)
  );

  digitalWrite(Config::ADXL_CS, LOW);
    /*
    Escritura de un registro:
    bit 7 = 0
    bit 6 = 0
  */
  spi_.transfer(reg & 0x3F);
  spi_.transfer(value);

  digitalWrite(Config::ADXL_CS, HIGH);

  spi_.endTransaction();
}

uint8_t ADXL345Driver::readRegister(uint8_t reg) {
  uint8_t value = 0;
  readRegisters(reg, 1, &value);
  return value;
}

void ADXL345Driver::readRegisters(
    uint8_t startReg,
    uint8_t numBytes,
    uint8_t *buffer
) {
  if (
      buffer == nullptr ||
      numBytes == 0
  ) {
    return;
  }

  // Bit 7: lectura.
  uint8_t address =
      startReg | 0x80;

  // Bit 6: operación multibyte.
  if (numBytes > 1) {
    address |= 0x40;
  }

  spi_.beginTransaction(
      SPISettings(
          Config::ADXL_SPI_HZ,
          MSBFIRST,
          SPI_MODE3
      )
  );

  digitalWrite(
      Config::ADXL_CS,
      LOW
  );

  spi_.transfer(address);

  for (
      uint8_t i = 0;
      i < numBytes;
      i++
  ) {
    buffer[i] =
        spi_.transfer(0x00);
  }

  digitalWrite(
      Config::ADXL_CS,
      HIGH
  );

  spi_.endTransaction();
}

void ADXL345Driver::readRaw(
    int16_t &x,
    int16_t &y,
    int16_t &z
) {
  uint8_t data[6] = {};

  readRegisters(
      REG_DATAX0,
      sizeof(data),
      data
  );

  x = static_cast<int16_t>(
      (
          static_cast<uint16_t>(
              data[1]
          ) << 8
      ) |
      data[0]
  );

  y = static_cast<int16_t>(
      (
          static_cast<uint16_t>(
              data[3]
          ) << 8
      ) |
      data[2]
  );

  z = static_cast<int16_t>(
      (
          static_cast<uint16_t>(
              data[5]
          ) << 8
      ) |
      data[4]
  );
}
