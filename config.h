#pragma once

#include <Arduino.h>

namespace Config {

    // ===========================================================================
    // Bus VSPI -> ADXL345 
    // ===========================================================================

    //  constexpr permite utilizar estos valores para:
    //- Dimensionar arrays estáticos.
    //- Comprobar configuraciones con static_assert.
    //- Evitar variables modificables durante la ejecución.
    // ===========================================================================
    constexpr int ADXL_SCK  = 18;                                                                                                // Pin del reloj SPI.
    constexpr int ADXL_MISO = 19;                                                                                                // Pin de datos del ADXL345 hacia el ESP32.
    constexpr int ADXL_MOSI = 23;                                                                                                // Pin de datos del ESP32 hacia el ADXL345.
    constexpr int ADXL_CS   = 5;                                                                                                 // Pin de selección del ADXL345.   
    
    // ===========================================================================                                                                
    // Bus HSPI -> SD                                                               
    // ===========================================================================                                                                
    constexpr int SD_SCK  = 14;                                                                                                  // Pin del reloj SPI.
    constexpr int SD_MISO = 27;                                                                                                  // Pin de datos de la SD hacia el ESP32.
    constexpr int SD_MOSI = 13;                                                                                                  // Pin de datos del ESP32 hacia la SD.
    constexpr int SD_CS   = 25;                                                                                                  // Pin de selección de la SD.

    // ===========================================================================
    // ADXL345
    //
    // IMPORTANTE:
    // ADXL_BW_RATE_VALUE y NOMINAL_SAMPLE_RATE_HZ deben representar
    // la misma frecuencia.
    //
    // Si se cambia el registro BW_RATE, también debe actualizarse
    // ===========================================================================

    constexpr uint32_t ADXL_SPI_HZ = 5000000UL;
    constexpr uint8_t ADXL_BW_RATE_VALUE = 0x0A;                                                                                        // 0x0A -> ODR 100 Hz, ancho de banda nominal 50 Hz
    constexpr uint8_t ADXL_DATA_FORMAT_VALUE = 0x09;                                                                                    // FULL_RES=1, justify=0, range=01 (+/-4 g) -> 0x09.
    constexpr float G_PER_LSB = 0.0039f;                                                                                                //
    constexpr float NOMINAL_SAMPLE_RATE_HZ = 100.0f;                                                                                    // Frecuencia de muestreo que se espera del sensor .

    // ===========================================================================
    // Promedios
    // ===========================================================================
    
    constexpr uint32_t REPORT_PERIOD_MS = 1000UL;                                                                                       // Cada segundo que se muestrea y se guarda el promedio.

    // ===========================================================================
    // FFT
    // ===========================================================================

    constexpr uint16_t FFT_SIZE = 256;                                                                                               // Debe ser potencia de dos.Tamaño del bloque de muestras .
    constexpr float MIN_ANALYSIS_FREQ_HZ = 1.0f;                                                                                     // Minimo de Hz desde los que se aplica el análisis de picos.
    constexpr float MAX_ANALYSIS_FREQ_HZ = 45.0f;                                                                                    // Máximo de Hz desde los que se aplica el análisis de picos.

    // ===========================================================================
    // Archivos
    // ===========================================================================

    constexpr char AVERAGE_LOG_PATH[]   = "/adxl_average.csv";                                                                        // Archivo de guardado de: marca temporal, media X, media Y, media Z, número de muestras, overruns, heap libre                                              [Datos por segundo]
    constexpr char VIBRATION_LOG_PATH[] = "/vibration_fft.csv";                                                                       // Archivo de guardado de: Freq efectiva de muestreo, resolución, RMS, picos, factor cresta, freq. dominante, amplitud dominante, eje dominante, heap libre [Datos cada 2.56 segundos]

    // ===========================================================================
    // Validaciones de configuración
    // ===========================================================================
    
    static_assert(
        FFT_SIZE >= 4 &&
            (FFT_SIZE & (FFT_SIZE - 1)) == 0,
        "FFT_SIZE debe ser una potencia de dos y al menos 4"
    );

    static_assert(
        MIN_ANALYSIS_FREQ_HZ >= 0.0f,
        "MIN_ANALYSIS_FREQ_HZ no puede ser negativa"
    );

    static_assert(
        MIN_ANALYSIS_FREQ_HZ <= MAX_ANALYSIS_FREQ_HZ,
        "La frecuencia minima no puede superar la maxima"
    );

    static_assert(
        MAX_ANALYSIS_FREQ_HZ <
            NOMINAL_SAMPLE_RATE_HZ / 2.0f,
        "MAX_ANALYSIS_FREQ_HZ debe ser menor que Nyquist"
    );
}