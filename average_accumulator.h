#pragma once

#include "measurement_types.h"

/*
===================================================================================================================================================
#  Acumula muestras crudas durante un intervalo.                                                                                                  #                                        
#                                                                                                                                                 #
#  takeResult():                                                                                                                                  #        
#  - Calcula la media de cada eje.                                                                                                                #                        
#  - Convierte el resultado de LSB a g.                                                                                                           #                            
#  - Copia el número de muestras y overruns.                                                                                                      #                                    
#  - Reinicia los acumuladores para el siguiente intervalo.                                                                                       #                                                
#  Para cada eje se acumulan las muestras crudas:                                                                                                 #
#                                                                                                                                                 #
#               Sx ​= (n =0) (N-1 )∑ ​x[n]                                                                                                          #
#  Después se calcula la media y se convierte a unidades de g:                                                                                    #    
#               x_media_g​= (Sx/N) ​​⋅K                                                                                                               #
#  Donde:                                                                                                                                         #
#  - N es el número de muestras                                                                                                                   #
#  - K = 0.0039 g/LSB                                                                                                                             #
====================================================================================================================================================
*/


class AverageAccumulator {
 public:
  void addSample(const AccelSample &sample);   


  AverageResult takeResult(                                    // Construye el promedio del intervalo
      uint32_t timestampMs,                                    // Para cada eje: media_g = (suma_de_muestras / número_de_muestras) * G_LSB
      uint32_t overruns                                        // La conversión a g se realiza después de sumar para evitar operaciones de coma flotante. 
  );                                                           // Posteriormente reinicia sumas y contador para que el siguiente resultado solo contenga las muestras del nuevo intervalo.

  void reset();

 private:
  int64_t accX_ = 0;
  int64_t accY_ = 0;
  int64_t accZ_ = 0;

  uint32_t sampleCount_ = 0;
};