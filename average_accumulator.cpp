#include "average_accumulator.h"

#include "config.h"

/*
==========================================================================
#       Incorpora una muestra a las sumas del intervalo.                 #                                                       
#                                                                        # 
#   Utiliza:                                                             #           
#    Sum_axis = sum(raw_axis[n])                                         #                               
#                                                                        #   
#   Cada llamada añade un término a la suma de su eje y aumenta N.       #                                                                 
==========================================================================
*/
void AverageAccumulator::addSample( const AccelSample &sample ) {

  accX_ += sample.x;
  accY_ += sample.y;
  accZ_ += sample.z;

  sampleCount_++;

}
/*
==========================================================================
#       Calcula las medias, construye el resultado y reinicia el estado. #                                                      
#                                                                        # 
#   Utiliza:                                                             #           
#    average_g = (S / N ) * K                                            #                               
#                                                                        #   
#   Realiza al final una conversión a coma flotante para                 #                                                                 
#   para evitar convertir cada muestra individualmente.                  #    
==========================================================================
*/
AverageResult AverageAccumulator::takeResult( uint32_t timestampMs, uint32_t overruns ) {

  AverageResult result;

  result.timestampMs = timestampMs;
  result.sampleCount = sampleCount_;
  result.overruns = overruns;

  if (sampleCount_ > 0) {
    
    result.xG = ( static_cast<float>(accX_) / sampleCount_ ) * Config::G_PER_LSB;
    result.yG = ( static_cast<float>(accY_) / sampleCount_ ) * Config::G_PER_LSB;
    result.zG = ( static_cast<float>(accZ_) / sampleCount_ ) * Config::G_PER_LSB;

  }

  reset();

  return result;
}

/*
==========================================================================
#       Pone a cero las sumas y reinicia el contador.                    #                                                      
==========================================================================
*/
void AverageAccumulator::reset() {

  accX_ = 0;
  accY_ = 0;
  accZ_ = 0;

  sampleCount_ = 0;

}