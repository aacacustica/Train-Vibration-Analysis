#include <Arduino.h>
#include <SPI.h>

#include "config.h"
#include "measurement_types.h"

#include "adxl345_driver.h"
#include "average_accumulator.h"
#include "vibration_analyzer.h"
#include "sd_logger.h"


/* 
=====================================================================================================================================================================================
#  ESP32 + ADXL345 + SD + análisis de vibraciones                                                                                                                                   #
#                                                                                                                                                                                   #
#                                                                                                                                                                                   #
#      ADXL345                                                                                                                                                                      #
#         |                                                                                                                                                                         #
#         +---> AverageAccumulator                                                                                                                                                  #
#         |       Produce una aceleración media cada segundo.                                                                                                                       #
#         |                                                                                                                                                                         #
#         +---> VibrationAnalyzer                                                                                                                                                   #
#                 Reúne bloques de FFT_SIZE muestras y calcula:                                                                                                                     #
#                 - RMS de vibración.                                                                                                                                               #
#                 - Pico a pico.                                                                                                                                                    #
#                 - Factor de cresta.                                                                                                                                               #
#                 - Frecuencia dominante.                                                                                                                                           #
#                 - Amplitud RMS de la frecuencia dominante.                                                                                                                        #
#                                                                                                                                                                                   #
#                                                                                                                             #
#  - ADXL345Driver adquiere muestras.                                                                                                                                               #
#  - AverageAccumulator calcula promedios.                                                                                                                                          #
#  - VibrationAnalyzer realiza el análisis matemático.                                                                                                                              #
#  - SDLogger guarda los resultados.                                                                                                                                                #
=====================================================================================================================================================================================
*/ 

// ============================================================
// Buses
// ============================================================
SPIClass spiADXL(VSPI);
SPIClass spiSD(HSPI);

// ============================================================
// Módulos
// ============================================================
ADXL345Driver sensor(spiADXL);

AverageAccumulator averageAccumulator;

VibrationAnalyzer vibrationAnalyzer;

SDLogger logger(spiSD);

// ============================================================
// Estado del coordinador
// ============================================================
uint32_t lastAverageReportMs = 0;

// ============================================================
// Prototipos locales del sketch
// ============================================================
void printAverageResult(const AverageResult &result);

void printVibrationReport(const VibrationReport &report,uint32_t totalOverruns);

// ============================================================
// SETUP
// ============================================================
void setup() {

    /*
  Secuencia de inicialización:

  1. Inicia el puerto serie.
  2. Configura VSPI para el ADXL345 y HSPI para la tarjeta SD.
  3. Inicializa y comprueba el acelerómetro.
  4. Reserva una única vez los recursos internos de la FFT.
  5. Inicializa la tarjeta y crea los CSV si no existen.
  6. Guarda el instante desde el que se medirá el primer intervalo
     de promedio.
*/
  Serial.begin(115200);

  delay(1000);

  Serial.println();
  Serial.println("================================================");
  Serial.println("ESP32 + ADXL345 + SD"" + analisis de vibraciones");
  Serial.println("================================================");

  // ----------------------------------------------------------
  // Inicializar los buses físicos
  // ----------------------------------------------------------

  spiADXL.begin(Config::ADXL_SCK,Config::ADXL_MISO,Config::ADXL_MOSI,Config::ADXL_CS);
  spiSD.begin(Config::SD_SCK,Config::SD_MISO,Config::SD_MOSI,Config::SD_CS);

  // ----------------------------------------------------------
  // Sensor
  // ----------------------------------------------------------

  if (sensor.begin()) {Serial.println("ADXL345 listo por VSPI");} 
  else {Serial.println("ERROR inicializando ADXL345");}

  // ----------------------------------------------------------
  // FFT
  // ----------------------------------------------------------
  Serial.printf("Heap antes de FFT:"" %u bytes\n",ESP.getFreeHeap());

  if (vibrationAnalyzer.begin()) {
    
    const float nominalResolution =Config::NOMINAL_SAMPLE_RATE_HZ /Config::FFT_SIZE;
    const float nominalDuration =Config::FFT_SIZE / Config::NOMINAL_SAMPLE_RATE_HZ;
    Serial.printf("FFT lista:"" N=%u,"" bloque=%.2f s,"" resolucion nominal=%.4f Hz\n",static_cast<unsigned int>(Config::FFT_SIZE),nominalDuration,nominalResolution);} 

  else { Serial.println("ERROR inicializando FFT"); }

  Serial.printf( "Heap despues de FFT:" " %u bytes\n", ESP.getFreeHeap() );

  // ----------------------------------------------------------
  // Tarjeta SD
  // ----------------------------------------------------------
  if (logger.begin()) {logger.listRoot();} 
  else { Serial.println( "ERROR inicializando SD" ); }
  lastAverageReportMs = millis();
}

// ============================================================
// LOOP
// ============================================================
void loop() {
/*
==========================================================================
#  Ciclo principal:                                                      #
#                                                                        #
#  1. Consulta DATA_READY del ADXL345.                                   #
#  2. Si existe una muestra nueva:                                       #
#       a. La añade al promedio del intervalo.                           #
#       b. La añade al bloque FFT.                                       #
#  3. Si la FFT completa un bloque, imprime y guarda el informe.         #
#  4. Cada REPORT_PERIOD_MS obtiene, imprime y guarda el promedio.       #
#  5. Cede tiempo a las tareas internas del ESP32 mediante yield().      #
==========================================================================
*/
  AccelSample sample;

  // ----------------------------------------------------------
  // Adquisición
  // ----------------------------------------------------------
  if ( sensor.readSampleIfReady(sample)) {
    // Camino 1: promedio.
    averageAccumulator.addSample(sample );

    // Camino 2: FFT.
    VibrationReport vibrationReport;

    if ( vibrationAnalyzer.addSample( sample, vibrationReport )) {
      
    const uint32_t totalOverruns = sensor.getTotalOverruns();
      printVibrationReport( vibrationReport, totalOverruns );
      if ( logger.isReady() && !logger.appendVibration( vibrationReport, totalOverruns )) { Serial.println( "Error escribiendo" " vibration_fft.csv" ); }

    }
  }

  // ----------------------------------------------------------
  // Informe promedio periódico
  // ----------------------------------------------------------
  const uint32_t nowMs = millis();

  if (static_cast<uint32_t>(nowMs - lastAverageReportMs) >= Config::REPORT_PERIOD_MS) {
    
    const uint32_t elapsedPeriods =( nowMs - lastAverageReportMs ) / Config::REPORT_PERIOD_MS;
    lastAverageReportMs += elapsedPeriods * Config::REPORT_PERIOD_MS;
    const AverageResult result = averageAccumulator.takeResult( nowMs, sensor.takeIntervalOverruns() );
    printAverageResult(result);
    if ( logger.isReady() && !logger.appendAverage(result) ) { Serial.println( "Error escribiendo" " adxl_average.csv");}

  }

  yield();
}

// ============================================================
// SALIDA SERIE DEL PROMEDIO
// ============================================================
void printAverageResult(const AverageResult &result) {
  if (result.sampleCount == 0) { Serial.printf(
        "AVG t=%lu ms"
        " | sin muestras"
        " | overruns=%lu"
        " | heap=%u\n",
        static_cast<unsigned long>( result.timestampMs),
        static_cast<unsigned long>( result.overruns),
        ESP.getFreeHeap()
    );

    return;
  }

  Serial.printf(
      "AVG t=%lu ms"
      " | X=%+.4f g"
      " Y=%+.4f g"
      " Z=%+.4f g"
      " | n=%lu"
      " overruns=%lu"
      " heap=%u\n",
      static_cast<unsigned long>(result.timestampMs),
      result.xG,
      result.yG,
      result.zG,
      static_cast<unsigned long>(result.sampleCount),
      static_cast<unsigned long>(result.overruns),
      ESP.getFreeHeap()
  );
}

// ============================================================
// SALIDA SERIE DE LA FFT
// ============================================================
void printVibrationReport(const VibrationReport &report, uint32_t totalOverruns) {
  Serial.println();

  Serial.printf(
      "FFT #%lu"
      " | Fs=%.3f Hz"
      " | dF=%.4f Hz"
      " | bloque=%.3f s\n",
      static_cast<unsigned long>(report.analysisId),
      report.effectiveSampleRateHz,
      report.resolutionHz,
      report.blockDurationSeconds
  );

  Serial.printf(
      "  X:"
      " RMS=%.5f g"
      "  P-P=%.5f g"
      "  Cresta=%.2f"
      "  Pico=%.3f Hz"
      " @ %.5f g RMS\n",
      report.x.rmsG,
      report.x.peakToPeakG,
      report.x.crestFactor,
      report.x.peakFrequencyHz,
      report.x.peakAmplitudeRmsG
  );

  Serial.printf(
      "  Y:"
      " RMS=%.5f g"
      "  P-P=%.5f g"
      "  Cresta=%.2f"
      "  Pico=%.3f Hz"
      " @ %.5f g RMS\n",
      report.y.rmsG,
      report.y.peakToPeakG,
      report.y.crestFactor,
      report.y.peakFrequencyHz,
      report.y.peakAmplitudeRmsG
  );

  Serial.printf(
      "  Z:"
      " RMS=%.5f g"
      "  P-P=%.5f g"
      "  Cresta=%.2f"
      "  Pico=%.3f Hz"
      " @ %.5f g RMS\n",
      report.z.rmsG,
      report.z.peakToPeakG,
      report.z.crestFactor,
      report.z.peakFrequencyHz,
      report.z.peakAmplitudeRmsG
  );

  Serial.printf(
      "  DOMINANTE:"
      " eje %c,"
      " %.3f Hz,"
      " %.5f g RMS"
      " | overruns total=%lu"
      " | heap=%u\n\n",
      report.dominantAxis,
      report.dominantFrequencyHz,
      report.dominantAmplitudeRmsG,
      static_cast<unsigned long>(totalOverruns),
      ESP.getFreeHeap()
  );
}