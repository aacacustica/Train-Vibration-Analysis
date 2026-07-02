#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

bool sdOK   = false;                                                                                                                // Indica si SD está disponible.

bool initSDCard() {                                                                                                                           // Inicializa la SD. SD_CS. bus spiSD. Si falla devuelve false
  if (!SD.begin(SD_CS, spiSD)) {
    Serial.println("Fallo al inicializar la SD por HSPI");
    return false;
  }

  const uint8_t cardType = SD.cardType();                                                                                                     // Distingue entre MMC,SDSC,SDHC.Ninguna tarjeta
  if (cardType == CARD_NONE) {
    Serial.println("No hay tarjeta SD");
    return false;
  }

  Serial.print("Tipo de tarjeta: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  const uint64_t cardSizeMb = SD.cardSize() / (1024ULL * 1024ULL);                                                                            // Tamaño en bytes. Se divide por 1024 * 1024 para obtener MiB
  Serial.printf("Tamano SD: %llu MB\n", cardSizeMb);

  const bool averageHeaderOK = ensureCSVHeader(                                                                                               // Cabeceras CSV. Para cada archivo timestamp_ms,x_avg_g,y_avg_g,z_avg_g,samples,overruns,heap_free
      AVERAGE_LOG_PATH,
      "timestamp_ms,x_avg_g,y_avg_g,z_avg_g,samples,overruns,heap_free"
  );

  const bool vibrationHeaderOK = ensureCSVHeader(                                                                                             
      VIBRATION_LOG_PATH,
      "timestamp_ms,analysis_id,fs_hz,resolution_hz,"
      "x_rms_g,x_p2p_g,x_crest,x_peak_hz,x_peak_rms_g,"
      "y_rms_g,y_p2p_g,y_crest,y_peak_hz,y_peak_rms_g,"
      "z_rms_g,z_p2p_g,z_crest,z_peak_hz,z_peak_rms_g,"
      "dominant_axis,dominant_peak_hz,dominant_peak_rms_g,"
      "overruns_total,heap_free"
  );

  return averageHeaderOK && vibrationHeaderOK;                                                                                                // La inicialización se considera correcta tras las anteriores comprobaciones si ambos archivos están disponibles con sus respectivas cabeceras.
}

bool ensureCSVHeader(const char *path, const char *header) {
  if (SD.exists(path)) {
    Serial.printf("CSV existente: %s\n", path);
    return true;
  }

  File file = SD.open(path, FILE_WRITE);
  if (!file) {
    Serial.printf("No se pudo crear: %s\n", path);
    return false;
  }

  file.println(header);
  file.close();
  Serial.printf("CSV creado: %s\n", path);
  return true;
}

bool appendVibrationCSV(
    uint32_t timestampMs,
    float effectiveSampleRateHz,
    float resolutionHz,
    const VibrationResult &xResult,
    const VibrationResult &yResult,
    const VibrationResult &zResult,
    const VibrationResult &dominant
) {
  File file = SD.open(VIBRATION_LOG_PATH, FILE_APPEND);
  if (!file) {
    return false;
  }

  file.print(timestampMs);
  file.print(',');
  file.print(fftAnalysisId);
  file.print(',');
  file.print(effectiveSampleRateHz, 6);
  file.print(',');
  file.print(resolutionHz, 6);
  file.print(',');

  file.print(xResult.rmsG, 6);
  file.print(',');
  file.print(xResult.peakToPeakG, 6);
  file.print(',');
  file.print(xResult.crestFactor, 4);
  file.print(',');
  file.print(xResult.peakFrequencyHz, 6);
  file.print(',');
  file.print(xResult.peakAmplitudeRmsG, 6);
  file.print(',');

  file.print(yResult.rmsG, 6);
  file.print(',');
  file.print(yResult.peakToPeakG, 6);
  file.print(',');
  file.print(yResult.crestFactor, 4);
  file.print(',');
  file.print(yResult.peakFrequencyHz, 6);
  file.print(',');
  file.print(yResult.peakAmplitudeRmsG, 6);
  file.print(',');

  file.print(zResult.rmsG, 6);
  file.print(',');
  file.print(zResult.peakToPeakG, 6);
  file.print(',');
  file.print(zResult.crestFactor, 4);
  file.print(',');
  file.print(zResult.peakFrequencyHz, 6);
  file.print(',');
  file.print(zResult.peakAmplitudeRmsG, 6);
  file.print(',');

  file.print(dominant.axis);
  file.print(',');
  file.print(dominant.peakFrequencyHz, 6);
  file.print(',');
  file.print(dominant.peakAmplitudeRmsG, 6);
  file.print(',');
  file.print(overrunTotal);
  file.print(',');
  file.println(ESP.getFreeHeap());

  file.close();
  return true;
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Directorio: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("No se pudo abrir el directorio");
    return;
  }

  if (!root.isDirectory()) {
    Serial.println("La ruta no es un directorio");
    root.close();
    return;
  }

  File file = root.openNextFile();

  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());

      if (levels > 0) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }

    file.close();
    file = root.openNextFile();
  }

  root.close();
}
