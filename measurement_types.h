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





//========================================================================================================================
//#                                     Muestra cruda obtenida del ADXL345                                               #
//# -Los tres ejes proceden de una misma lectura multibyte por lo que representan el mismo instante de adquisición       #
//========================================================================================================================
struct AccelSample {
  int16_t x = 0;                                                // Lectura cruda X en LSB.
  int16_t y = 0;                                                // Lectura cruda Y en LSB.
  int16_t z = 0;                                                // Lectura cruda Z en LSB.
  uint32_t timestampUs = 0;                                     // Instante de adquisición en µs.
};

//========================================================================================================================
//#                                     Instante en el que termina el intervalo                                          #
//#   No es la marca temporal de una muestra concreta. Representa el momento en que se generó el informe.                #
//========================================================================================================================
struct AverageResult {
  uint32_t timestampMs = 0;                                     // Instante en ms en el que termina el intervalo.

  float xG = 0.0f;                                              // Aceleración media del eje X.
  float yG = 0.0f;                                              // Aceleración media del eje Y.
  float zG = 0.0f;                                              // Aceleración media del eje Z.

  uint32_t sampleCount = 0;                                     // Número de muestras del promedio.
  uint32_t overruns = 0;                                        // Overruns durante el intervalo. Un overrun indica que el sensor generó una nueva mueustra antes de que el programa leyera la anterior
};

//========================================================================================================================
//#                                     Resultado temporal y espectral de un único eje                                   #
//#   Se calcula sobre un bloque de Config::FFT_SIZE muestras.                                                           #
//========================================================================================================================
struct AxisVibrationResult {
  char axis = '?';                                              // Eje actual

  float meanG = 0.0f;                                           // Componente media en g.
  float rmsG = 0.0f;                                            // RMS total sin componente DC en g.
  float peakToPeakG = 0.0f;                                     // Máximo menos mínimo en g.
  float crestFactor = 0.0f;                                     // Magnitud adimensional.

  float peakFrequencyHz = 0.0f;                                 // Frecuencia dominante en Hz.
  float peakAmplitudeRmsG = 0.0f;                               // Amplitud RMS del pico en g.
};

//========================================================================================================================
//#                                     Informe completo generado para un bloque FFT                                     #
//#   Agrupa la información temporal del bloque y los resultados de los tres ejes                                        #
//#                                                                                                                      # 
//#   Fs = (N - 1) * 1e6 / elapsedUs                                                                                     #                               
//#   resolution = Fs / N                                                                                                #                   
//#   blockDuration = (N - 1) / Fs                                                                                       #                             
//#                                                                                                                      # 
//#   Donde:                                                                                                             #       
//#   - Fs = frecuencia de muestreo efectiva.                                                                            #                                       
//#   - N  = número de muestras del bloque.                                                                              #                                     
//#   - elapsedUs = tiempo entre primera y última muestra en µs.                                                         #                                                           
//========================================================================================================================
struct VibrationReport {
  uint32_t timestampMs = 0;                                     // Instante en el que finaliza el análisis. En milisegundos. Se obtiene con millis() cuando se completa el bloque.
  uint32_t analysisId = 0;                                      // Identificador consecutivo del análisis. Comienza en 1 y aumenta una unidad por bloque procesado. 

  float effectiveSampleRateHz = 0.0f;                           // Frecuencia de muestreo estimada a partir de las marcas temporales.
  float resolutionHz = 0.0f;                                    // Separación frecuencial entre bins de la FFT.
  float blockDurationSeconds = 0.0f;                            // Tiempo entre la primera y la última muestra del bloque

  AxisVibrationResult x;                                        // Resultado del eje X.
  AxisVibrationResult y;                                        // Resultado del eje Y.
  AxisVibrationResult z;                                        // Resultado del eje Z.

  char dominantAxis = '?';                                      // Eje con la mayor amplitud RMS de pico espectral.
  float dominantFrequencyHz = 0.0f;                             // Frecuencia dominante del eje seleccionado.
  float dominantAmplitudeRmsG = 0.0f;                           // Amplitud RMS del pico dominante seleccionado.
};

