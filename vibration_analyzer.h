#pragma once

#include <Arduino.h>
#include "fft.h"

#include "config.h"
#include "measurement_types.h"

/*
===================================================================================================================================================
#  Analizador de vibraciones por bloques.                                                                                                         #
#                                                                                                                                                 #
#  El módulo:                                                                                                                                     #
#  1. Reúne FFT_SIZE muestras sincronizadas de X, Y y Z.                                                                                          #
#  2. Calcula la frecuencia de muestreo efectiva del bloque.                                                                                      #
#  3. Elimina la media de cada eje.                                                                                                               #
#  4. Obtiene métricas temporales.                                                                                                                #
#  5. Aplica una ventana Hann.                                                                                                                    #
#  6. Ejecuta una FFT por eje.                                                                                                                    #
#  7. Busca el pico dominante dentro de la banda configurada.                                                                                     #
#  8. Devuelve los resultados mediante VibrationReport.                                                                                           #
#                                                                                                                                                 #
#  No imprime resultados y no escribe en la tarjeta SD.                                                                                           #
====================================================================================================================================================
*/


class VibrationAnalyzer {
 public:
  VibrationAnalyzer() = default;
  ~VibrationAnalyzer();
  VibrationAnalyzer( const VibrationAnalyzer & ) = delete;
  VibrationAnalyzer &operator=( const VibrationAnalyzer & ) = delete;

  bool begin();

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

  float getBinAmplitudeRmsG( uint16_t bin ) const;

  int16_t samplesX_[ Config::FFT_SIZE ] = {};

  int16_t samplesY_[ Config::FFT_SIZE ] = {};

  int16_t samplesZ_[ Config::FFT_SIZE ] = {};

  uint16_t sampleIndex_ = 0;

  uint32_t firstSampleUs_ = 0;
  uint32_t lastSampleUs_ = 0;
  uint32_t analysisId_ = 0;

  float fftInput_[ Config::FFT_SIZE ] = {};

  float fftOutput_[ Config::FFT_SIZE ] = {};

  float hannWindow_[ Config::FFT_SIZE ] = {};

  float hannWindowSum_ = 0.0f;

  fft_config_t *fftPlan_ = nullptr;

  bool ready_ = false;
};