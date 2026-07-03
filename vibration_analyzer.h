#pragma once

#include <Arduino.h>
#include "fft.h"

#include "config.h"
#include "measurement_types.h"

class VibrationAnalyzer {
 public:
  VibrationAnalyzer() = default;

  ~VibrationAnalyzer();

  /*
    El plan FFT contiene recursos internos, por lo que
    no permitimos copiar el objeto.
  */
  VibrationAnalyzer(
      const VibrationAnalyzer &
  ) = delete;

  VibrationAnalyzer &operator=(
      const VibrationAnalyzer &
  ) = delete;

  bool begin();

  /*
    Añade una muestra.

    Devuelve true cuando se han reunido FFT_SIZE
    muestras y report contiene un análisis nuevo.
  */
  bool addSample(
      const AccelSample &sample,
      VibrationReport &report
  );

 private:
  void initializeHannWindow();

  void analyzeBlock(
      uint32_t timestampMs,
      VibrationReport &report
  );

  AxisVibrationResult analyzeAxis(
      const int16_t *samples,
      char axis,
      float effectiveSampleRateHz
  );

  float getBinAmplitudeRmsG(
      uint16_t bin
  ) const;

  // Muestras crudas.
  int16_t samplesX_[
      Config::FFT_SIZE
  ] = {};

  int16_t samplesY_[
      Config::FFT_SIZE
  ] = {};

  int16_t samplesZ_[
      Config::FFT_SIZE
  ] = {};

  uint16_t sampleIndex_ = 0;

  uint32_t firstSampleUs_ = 0;
  uint32_t lastSampleUs_ = 0;
  uint32_t analysisId_ = 0;

  // Buffers reutilizados por los tres ejes.
  float fftInput_[
      Config::FFT_SIZE
  ] = {};

  float fftOutput_[
      Config::FFT_SIZE
  ] = {};

  float hannWindow_[
      Config::FFT_SIZE
  ] = {};

  float hannWindowSum_ = 0.0f;

  fft_config_t *fftPlan_ = nullptr;

  bool ready_ = false;
};