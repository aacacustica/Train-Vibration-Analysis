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
  uint32_t timestampMs = 0;

  float xG = 0.0f;
  float yG = 0.0f;
  float zG = 0.0f;

  uint32_t sampleCount = 0;
  uint32_t overruns = 0;
};

struct AxisVibrationResult {
  char axis = '?';

  float meanG = 0.0f;
  float rmsG = 0.0f;
  float peakToPeakG = 0.0f;
  float crestFactor = 0.0f;

  float peakFrequencyHz = 0.0f;
  float peakAmplitudeRmsG = 0.0f;
};

struct VibrationReport {
  uint32_t timestampMs = 0;
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

