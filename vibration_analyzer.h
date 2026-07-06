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
  VibrationAnalyzer() = default;                                                        // Construye el analizador con estado inicial vacío.
  ~VibrationAnalyzer();                                                                 // Destruye el plan FFT si fue creado.
  VibrationAnalyzer( const VibrationAnalyzer & ) = delete;                              // La clase no se puede copiar.
  VibrationAnalyzer &operator=( const VibrationAnalyzer & ) = delete;                   // La clase no admite asignación por copia.

  bool begin();                                                                         // Inicializa ventana, buffers y plan FFT. Devuelve true si fft_init() devuelve un plan FFT válido. False si no se pudo crear el plan FFT.

  bool addSample(                                                                       // Añade una muestra al bloque actual. Devuelve true cuando se completa y analiza un bloque. Devuelve False mientras todavía faltan muestras o al FFT no está inicializada de antes.
      const AccelSample &sample,                                                        // Muestra X/Y/Z con marca temporal.
      VibrationReport &report                                                           // Referencia de salida. Solo contiene un nuevo informe cuando la función devuelve true.
  );

 private:
  void initializeHannWindow();                                                          // Precalcula los coeficientes de la ventana Hann a utilizar en el análisis FFT.

  void analyzeBlock(                                                                    // Analiza un eje en los dominios temporal y frecuencial.
      uint32_t timestampMs,                                                             // Momento de finalización del análisis en Ms.
      VibrationReport &report                                                           // Referencia donde se escribirá el informe.
  );

  AxisVibrationResult analyzeAxis(                                                      // Analiza un eje en los dominios temporal y frecuencial.Devuelve métricas temporales y pico espectral dominante del eje.
      const int16_t *samples,                                                           // Puntero a las muestras crudas del eje.               
      char axis,                                                                        // Identificador del eje.
      float effectiveSampleRateHz                                                       // Frecuencia efectiva del bloque en Hz.
  );

  float getBinAmplitudeRmsG( uint16_t bin ) const;                                      // Convierte un bin complejo en amplitud RMS. @bin = bin índice del bin que se debe consultar. Devuelve amplitud RMS en g del bin.

  int16_t samplesX_[ Config::FFT_SIZE ] = {};                                           // Bloque de muestras crudas del eje X.

  int16_t samplesY_[ Config::FFT_SIZE ] = {};                                           // Bloque de muestras crudas del eje Y.

  int16_t samplesZ_[ Config::FFT_SIZE ] = {};                                           // Bloque de muestras crudas del eje Z.

  uint16_t sampleIndex_ = 0;                                                            // Posición donde se guardará la siguiente muestra.

  uint32_t firstSampleUs_ = 0;                                                          // Marca temporal de la primera muestra del bloque en microsegundos.
  uint32_t lastSampleUs_ = 0;                                                           // Marca temporal de la última muestra del bloque en microsegundos.
  uint32_t analysisId_ = 0;                                                             // Número de bloques analizados

  float fftInput_[ Config::FFT_SIZE ] = {};                                             // Buffer real de entrada de la FFT

  float fftOutput_[ Config::FFT_SIZE ] = {};                                            // Buffer de salida de la FFT

  float hannWindow_[ Config::FFT_SIZE ] = {};                                           // Coeficientes precalculados de la ventana Hann.

  float hannWindowSum_ = 0.0f;                                                          // Suma de los coeficientes Hann. Se utiliza para compensar la ganancia coherente de la ventana.

  fft_config_t *fftPlan_ = nullptr;                                                     // Plan interno utilizado por la biblioteca FFT.

  bool ready_ = false;                                                                  // Indica si el plan FFT está disponible.
};