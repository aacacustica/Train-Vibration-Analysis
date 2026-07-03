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
#  No calcula promedios ni resultados FFT.                                                                                  #                                        
#                                                                                                                           #
=============================================================================================================================
*/


class SDLogger {
 public:
  explicit SDLogger(SPIClass &spi);

  bool begin();

  bool isReady() const;

  bool appendAverage(
      const AverageResult &result
  );

  bool appendVibration(
      const VibrationReport &report,
      uint32_t totalOverruns
  );

  void listRoot();

 private:
  bool ensureCSVHeader(
      const char *path,
      const char *header
  );

  void listDirectory(
      const char *dirname,
      uint8_t levels
  );

  SPIClass &spi_;
  bool ready_ = false;
};