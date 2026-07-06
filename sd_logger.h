#pragma once

#include <Arduino.h>
#include <SPI.h>
#include "measurement_types.h"


/*
=============================================================================================================================
#                                                                                                                           #
#  Encapsula la persistencia en tarjeta SD.                                                                                 #                                        
#                                                                                                                           #
#  Responsabilidades:                                                                                                       #                    
#  - Inicializar la tarjeta en el bus SPI recibido.                                                                         #                                                
#  - Crear los CSV y sus cabeceras.                                                                                         #                                
#  - Añadir resultados de promedio y vibración.                                                                             #                                            
#  - Listar el contenido de la tarjeta para diagnóstico.                                                                    #                                                        
#                                                                                                                           #
#  No calcula promedios ni resultados FFT.Solo persiste en almacenamiento AverageResult y VibrationReport.                  #                                        
#                                                                                                                           #
=============================================================================================================================
*/


class SDLogger {
 public:
  explicit SDLogger(SPIClass &spi);                                                                                         // Construye el logger sobre un bus SPI existente.

  bool begin();                                                                                                             // Inicializa la tarjeta y prepara los archivos CSV. Devuelve true si la tarjeta y ambas cabeceras están disponibles. Devuelve false si falla la tarjeta o la creación de archivos.
  bool isReady() const;                                                                                                     // Indica si el logger está disponible. Devuelve true después de una inicialización correcta.
  bool appendAverage( const AverageResult &result);                                                                         // Añade una fila al CSV de promedios.@result = Resultado promedio que se debe guardar. Devuelve true si se escribió correctamente. 
  bool appendVibration( const VibrationReport &report, uint32_t totalOverruns );                                            // Añade una fila al CSV de vibración.@report = Resultado FFT que se debe guardar.@totalOverruns = Overruns acumulados desde el arranque. Devuelve true si se escribió correctamente.

  void listRoot();                                                                                                          // Lista el contenido del directorio raíz de la tarjeta.

 private:
  bool ensureCSVHeader( const char *path, const char *header );                                                             // Crea un CSV con su cabecera si no existe. @path = ruta absoluta del archivo. @header línea de nombres de columnas. Devuelve true si el archivo ya existía o si se escribió correctamente.

  void listDirectory( const char *dirname, uint8_t levels );                                                                // Lista recursivamente un directorio. @dirname = ruta del directorio. @levels = número máximo de niveles de recursión.

  SPIClass &spi_;                                                                                                           // Bus SPI utilizado por la tarjeta SD.
  bool ready_ = false;                                                                                                      // Estado de inicialización del logger.
};