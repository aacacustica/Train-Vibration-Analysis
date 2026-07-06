#pragma once

#include <Arduino.h>
#include <SPI.h>

#include "measurement_types.h"


/*
#==============================================================================================================================================================================================================================
#  Driver SPI del ADXL345.                                                                                                                                                                                                    #                          
#                                                                                                                                                                                                                             #
#  Responsabilidades:                                                                                                                                                                                                         #                    
#  - Configurar el sensor.                                                                                                                                                                                                    #                          
#  - Verificar su identificador.                                                                                                                                                                                              #                                
#  - Consultar DATA_READY.                                                                                                                                                                                                    #                          
#  - Leer simultáneamente X, Y y Z.                                                                                                                                                                                           #                                  
#  - Contabilizar eventos OVERRUN.                                                                                                                                                                                            #                                  
#                                                                                                                                                                                                                             #
#  El driver no calcula promedios, no ejecuta FFT y no accede                                                                                                                                                                 #                                                            
#  a la tarjeta SD.                                                                                                                                                                                                           #                  
#==============================================================================================================================================================================================================================
*/

class ADXL345Driver {
 public:
  explicit ADXL345Driver(SPIClass &spi);

  bool begin();                                                                     // Configura el sensor y comprueba su identidad.
  bool readSampleIfReady(AccelSample &sample);                                      // Consulta el registro INT_SOURCE.Devuelve false cuando el driver no se inicializó correctamente o el sensor todavía no ha generado una muestra nueva. Devuelve true cuando DATA_READY está activo, los seis bytes X/Y/Z han sido leídos y sample contiene los valores y su marca temporal. 
  uint32_t getTotalOverruns() const;                                                // Overruns acumulados desde el arranque.
  uint32_t takeIntervalOverruns();                                                  // Devuelve los overruns del intervalo actual y reinicia solamente el contador de intervalo.

 private:
  void writeRegister( uint8_t reg, uint8_t value );
  uint8_t readRegister(uint8_t reg);
  void readRegisters( uint8_t startReg, uint8_t numBytes, uint8_t *buffer );
  void readRaw( int16_t &x, int16_t &y, int16_t &z );

  SPIClass &spi_;
  bool ready_ = false;
  uint32_t intervalOverruns_ = 0;
  uint32_t totalOverruns_ = 0;
};