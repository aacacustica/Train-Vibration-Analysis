#pragma once

#include <Arduino.h>


/*
=============================================================================================================================
  Estructuras de intercambio entre módulos.

  Estos tipos permiten que cada componente se mantenga desacoplado:

  ADXL345Driver
      produce AccelSample.

  AverageAccumulator
      consume AccelSample y produce AverageResult.

  VibrationAnalyzer
      consume AccelSample y produce VibrationReport.

  SDLogger y las funciones de impresión
      consumen los resultados sin conocer cómo fueron calculados.

  Ejemplo:
     VibrationAnalyzer ──produce──> VibrationReport
     SDLogger          ──consume──> VibrationReport
=============================================================================================================================
*/

struct AccelSample {
  int16_t x = 0;                                                // Lectura cruda X en LSB.
  int16_t y = 0;                                                // Lectura cruda Y en LSB.
  int16_t z = 0;                                                // Lectura cruda Z en LSB.
  uint32_t timestampUs = 0;                                     // Instante de adquisición en µs.
};

struct AverageResult {
  uint32_t timestampMs = 0;

  float xG = 0.0f;
  float yG = 0.0f;
  float zG = 0.0f;

  uint32_t sampleCount = 0;
  uint32_t overruns = 0;
};

struct AxisVibrationResult {
  char axis = '?';                                              // Eje actual

  float meanG = 0.0f;                                           // Componente media en g.
  float rmsG = 0.0f;                                            // RMS total sin componente DC en g.
  float peakToPeakG = 0.0f;                                     // Máximo menos mínimo en g.
  float crestFactor = 0.0f;                                     // Magnitud adimensional.

  float peakFrequencyHz = 0.0f;                                 // Frecuencia dominante en Hz.
  float peakAmplitudeRmsG = 0.0f;                               // Amplitud RMS del pico en g.
};

struct VibrationReport {
  uint32_t timestampMs = 0;                                     //
  uint32_t analysisId = 0;

  float effectiveSampleRateHz = 0.0f;
  float resolutionHz = 0.0f;
  float blockDurationSeconds = 0.0f;

  AxisVibrationResult x;
  AxisVibrationResult y;
  AxisVibrationResult z;

  char dominantAxis = '?';
  float dominantFrequencyHz = 0.0f;
  float dominantAmplitudeRmsG = 0.0f;
};

