#include "config.h"
#include "adxl345_driver.h"

//========================
// Transforma muestras en objeto AverageResult y reinicia acumuladores
//========================
void AverageAccumulator::addSample( const AccelSample &sample ) {

  accX_ += sample.x;
  accY_ += sample.y;
  accZ_ += sample.z;

  sampleCount_++;

}

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

void AverageAccumulator::reset() {

  accX_ = 0;
  accY_ = 0;
  accZ_ = 0;

  sampleCount_ = 0;
  
}