#pragma once

#include <Arduino.h>

// =========================
// Estructuras comunes para intercambio de datos entre los módulos
// Ejemplo:
//      VibrationAnalyzer ──produce──> VibrationReport
//      SdLogger          ──consume──> VibrationReport
// =========================

struct AccelSample {
  int16_t x = 0;
  int16_t y = 0;
  int16_t z = 0;
  uint32_t timestampUs = 0;
};

struct AverageResult {
  uint32_t timestampMs;

  float xG;
  float yG;
  float zG;

  uint32_t sampleCount;
  uint32_t overruns;
};

struct AxisVibrationResult {
  char axis;

  float meanG;
  float rmsG;
  float peakToPeakG;
  float crestFactor;

  float peakFrequencyHz;
  float peakAmplitudeRmsG;
};

struct VibrationReport {
  uint32_t timestampMs;
  uint32_t analysisId;

  float effectiveSampleRateHz;
  float resolutionHz;
  float blockDurationSeconds;

  AxisVibrationResult x;
  AxisVibrationResult y;
  AxisVibrationResult z;

  char dominantAxis;
  float dominantFrequencyHz;
  float dominantAmplitudeRmsG;
};

