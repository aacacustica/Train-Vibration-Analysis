#pragma once

#include <Arduino.h>


// Muestras originales de los tres ejes.                                                                      
static int16_t fftSamplesX[FFT_SIZE];                                                                                               // Array de muestras FFT eje X size = 256.
static int16_t fftSamplesY[FFT_SIZE];                                                                                               // Array de muestras FFT eje Y size = 256.
static int16_t fftSamplesZ[FFT_SIZE];                                                                                               // Array de muestras FFT eje Z size = 256.
static uint16_t fftSampleIndex = 0;                                                                                                 // Índice del bloque en el que se guardará la siguiente muestra.

static uint32_t fftFirstSampleUs = 0;                                                                                               // Instante en ms de la primera muestra del bloque.
static uint32_t fftLastSampleUs  = 0;                                                                                               // Instante en ms de la última muestra del bloque.
static uint32_t fftAnalysisId    = 0;                                                                                               // Contador para numerar los análisis #FFT1,#FFT2,#FFT3...

// Un único plan y dos buffers, reutilizados para X, Y y Z.                                                                     
static float fftInput[FFT_SIZE];                                                                                                    // Contiene las muestras preparadas para la FFT.
static float fftOutput[FFT_SIZE];                                                                                                   // Contiene el resultado de la FFT.
static float hannWindow[FFT_SIZE];                                                                                                  // Contiene los 256 coeficientes de la ventana Hann.
static float hannWindowSum = 0.0f;                                                                                                  // Contiene la suma de todos los coeficientes de la ventana Hann.

fft_config_t *fftPlan = nullptr;                                                                                                    // Puntero a estructura de configuración interna de la bib.FFT.
bool fftOK = false;                                                                                                                 // Indica si la FFT se inicializó correctamente.
