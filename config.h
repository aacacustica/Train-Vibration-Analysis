#pragma once

#include <Arduino.h>

namespace Config {

    // =========================
    // Bus VSPI -> ADXL345 
    // static -> variáble limitada a este archivo
    // const  -> valor no cambiará durante la ejecución
    // =========================
    constexpr int ADXL_SCK  = 18;                                                                                                // Reloj SPI.
    constexpr int ADXL_MISO = 19;                                                                                                // Datos del ADXL345 hacia el ESP32.
    constexpr int ADXL_MOSI = 23;                                                                                                // Datos del ESP32 hacia el ADXL345.
    constexpr int ADXL_CS   = 5;                                                                                                 // Pin de selección del ADXL345.   
    
    // =========================                                                                
    // Bus HSPI -> SD                                                               
    // =========================                                                                
    constexpr int SD_SCK  = 14;                                                                
    constexpr int SD_MISO = 27;                                                                
    constexpr int SD_MOSI = 13;                                                                
    constexpr int SD_CS   = 25; 

    // =========================
    // ADXL345
    // =========================

    constexpr uint32_t ADXL_SPI_HZ = 5000000;
    constexpr uint8_t ADXL_BW_RATE_VALUE = 0x0A; // 0x0A -> ODR 100 Hz, ancho de banda nominal 50 Hz
    constexpr uint8_t ADXL_DATA_FORMAT_VALUE = 0x09;    // FULL_RES=1, justify=0, range=01 (+/-4 g) -> 0x09.
    constexpr float G_PER_LSB = 0.0039f;
    constexpr float NOMINAL_SAMPLE_RATE_HZ = 100.0f;                                                                                   // Frecuencia de muestreo que se espera del sensor .

    // =========================
    // Promedios
    // =========================
    
    constexpr uint32_t REPORT_PERIOD_MS = 1000;                                                                                        // Cada segundo que se muestrea y se guarda el promedio.

    // =========================
    // FFT
    // =========================

    constexpr uint16_t FFT_SIZE = 256;                                                                                               // Debe ser potencia de dos.Tamaño del bloque de muestras .
    constexpr float MIN_ANALYSIS_FREQ_HZ = 1.0f;                                                                                     // Minimo de Hz desde los que se aplica el análisis de picos.
    constexpr float MAX_ANALYSIS_FREQ_HZ = 45.0f;                                                                                    // Máximo de Hz desde los que se aplica el análisis de picos.

    // =========================
    // Archivos
    // =========================

    constexpr char AVERAGE_LOG_PATH[]   = "/adxl_average.csv";                                                                        // Archivo de guardado de: marca temporal, media X, media Y, media Z, número de muestras, overruns, heap libre                                              [Datos por segundo]
    constexpr char VIBRATION_LOG_PATH[] = "/vibration_fft.csv";                                                                       // Archivo de guardado de: Freq efectiva de muestreo, resolución, RMS, picos, factor cresta, freq. dominante, amplitud dominante, eje dominante, heap libre [Datos cada 2.56 segundos]

}