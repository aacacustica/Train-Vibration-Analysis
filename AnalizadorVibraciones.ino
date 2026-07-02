#include <Arduino.h>
#include <SPI.h>

#include "config.h"
#include "measurement_types.h"

#include "adxl345_driver.h"
#include "average_accumulator.h"
#include "vibration_analyzer.h"
#include "sd_logger.h"

SPIClass spiADXL(VSPI);
SPIClass spiSD(HSPI);

ADXL345Driver sensor(spiADXL);
AverageAccumulator averageAccumulator;
VibrationAnalyzer vibrationAnalyzer;
SDLogger logger(spiSD);

uint32_t lastAverageReportMs = 0;

void printAverageResult(
    const AverageResult &result
);

void printVibrationReport(
    const VibrationReport &report
);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println(
      "ESP32 + ADXL345 + analisis de vibraciones"
  );

  spiADXL.begin(
      Config::ADXL_SCK,
      Config::ADXL_MISO,
      Config::ADXL_MOSI,
      Config::ADXL_CS
  );

  spiSD.begin(
      Config::SD_SCK,
      Config::SD_MISO,
      Config::SD_MOSI,
      Config::SD_CS
  );

  if (!sensor.begin()) {
    Serial.println(
        "ERROR inicializando ADXL345"
    );
  }

  Serial.printf(
      "Heap antes de FFT: %u\n",
      ESP.getFreeHeap()
  );

  if (!vibrationAnalyzer.begin()) {
    Serial.println(
        "ERROR inicializando FFT"
    );
  }

  Serial.printf(
      "Heap despues de FFT: %u\n",
      ESP.getFreeHeap()
  );

  if (!logger.begin()) {
    Serial.println(
        "ERROR inicializando SD"
    );
  } else {
    logger.listRoot();
  }

  lastAverageReportMs = millis();
}

void loop() {
  AccelSample sample;

  if (sensor.readSampleIfReady(sample)) {
    averageAccumulator.addSample(sample);

    VibrationReport vibrationReport;

    if (
        vibrationAnalyzer.addSample(
            sample,
            vibrationReport
        )
    ) {
      printVibrationReport(vibrationReport);

      if (logger.isReady()) {
        logger.appendVibration(
            vibrationReport,
            sensor.getTotalOverruns()
        );
      }
    }
  }

  const uint32_t nowMs = millis();

  if (
      static_cast<uint32_t>(
          nowMs - lastAverageReportMs
      ) >= Config::REPORT_PERIOD_MS
  ) {
    const uint32_t elapsedPeriods =
        (nowMs - lastAverageReportMs) /
        Config::REPORT_PERIOD_MS;

    lastAverageReportMs +=
        elapsedPeriods *
        Config::REPORT_PERIOD_MS;

    const AverageResult result =
        averageAccumulator.takeResult(
            nowMs,
            sensor.takeIntervalOverruns()
        );

    printAverageResult(result);

    if (logger.isReady()) {
      logger.appendAverage(result);
    }
  }

  yield();
}

