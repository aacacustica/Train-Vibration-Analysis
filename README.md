# Train Vibration Analysis

## Objetivo

## Hardware
- Modelo exacto de ESP32
- ADXL345
- Módulo de tarjeta SD

## Conexiones
### ADXL345 / VSPI
### SD / HSPI

## Dependencias
- Versión del core ESP32 para Arduino
- Biblioteca esp32-fft y versión
- Arduino IDE o PlatformIO

## Arquitectura
- ADXL345Driver
- AverageAccumulator
- VibrationAnalyzer
- SDLogger

## Flujo de datos

## Configuración
- Frecuencia de muestreo
- Tamaño FFT
- Banda analizada
- Rango del sensor

## Archivos CSV
### adxl_average.csv
### vibration_fft.csv

## Fundamentos matemáticos
- Media
- RMS
- Ventana Hann
- FFT
- Resolución espectral
- Interpolación del pico

## Limitaciones actuales
- Análisis bloqueante
- Escritura SD bloqueante
- Posibles overruns
- Sin FIFO
- Sin solapamiento de bloques