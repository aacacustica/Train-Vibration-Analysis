#include "vibration_analyzer.h"
#include <math.h>

namespace {
  constexpr float TWO_PI_F = 6.28318530717958647692f;                                         
  constexpr float INV_SQRT_2_F = 0.70710678118654752440f;                                     
}  

VibrationAnalyzer::~VibrationAnalyzer(){
  if(fftPlan_ !=nullptr){
    fft_destroy(fftPlan_);
    fftPlan_ = nullptr;
  }
}

/*
#==============================================================================================================================================================================================================================
#       Reinicia el analizador y crea el plan FFT..                                                                                                                                                                           #                                                      
#                                                                                                                                                                                                                             # 
#   Proceso:                                                                                                                                                                                                                  #           
#    1. Reinicia índices y marcas temporales.                                                                                                                                                                                 #                               
#    2. Destruye un posible plan anterior.                                                                                                                                                                                    #   
#    3. Precalcula la ventana Hann.                                                                                                                                                                                           #   
#    4. Crea el plan FFT utilizando los buffers del objeto.                                                                                                                                                                   #
#==============================================================================================================================================================================================================================
*/
bool VibrationAnalyzer::begin() {
  ready_ = false;

  sampleIndex_ = 0;
  firstSampleUs_ = 0;
  lastSampleUs_ = 0;
  analysisId_ = 0;

  if (fftPlan_ != nullptr) {
    fft_destroy(fftPlan_);
    fftPlan_ = nullptr;
  }

  initializeHannWindow();

  fftPlan_ = fft_init(Config::FFT_SIZE,FFT_REAL,FFT_FORWARD,fftInput_,fftOutput_);

  ready_ = fftPlan_ != nullptr;

  return ready_;
}

bool VibrationAnalyzer::addSample(const AccelSample &sample, VibrationReport &report){
  if (!ready_) {
    return false;
  }
  if (sampleIndex_ == 0){
    firstSampleUs_ = sample.timestampUs;
  }

  samplesX_[sampleIndex_] = sample.x;
  samplesY_[sampleIndex_] = sample.y;
  samplesZ_[sampleIndex_] = sample.z;

  lastSampleUs_ = sample.timestampUs;

  sampleIndex_++;

  if(sampleIndex_ < Config::FFT_SIZE){ return false; }

  analyzeBlock(millis(),report);

  sampleIndex_ = 0;
  
  return true;
}
/*
#==============================================================================================================================================================================================================================
#       Precalcula la ventana Hann y su suma.                                                                                                                                                                                 #                                                      
#                                                                                                                                                                                                                             # 
#   Fórmula:                                                                                                                                                                                                                  #           
#     w[n] = 0.5 * (1 - cos(2pi * n / (N-1)))                                                                                                                                                                                 #
#   Para:                                                                                                                                                                                                                     # 
#     n = 0, 1, ---, N-1                                                                                                                                                                                                      #                 
#   La ventana lleva los extremos del bloque hacia cero para reducir                                                                                                                                                          #                                                             
#   la fuga espectral causada por la discontinuidad entre el final                                                                                                                                                            #                                                           
#   y el principio del bloque periódico supuesto por la FFT.                                                                                                                                                                  #                                                     
#==============================================================================================================================================================================================================================

*/
void VibrationAnalyzer::initializeHannWindow() {                                                                                                           
  hannWindowSum_ = 0.0f;                                                                                                                       

  for (uint16_t i = 0; i < Config::FFT_SIZE; i++) {                                                                                             
    const float phase = TWO_PI_F * static_cast<float>(i) / static_cast<float>(Config::FFT_SIZE - 1);                                            
    hannWindow_[i] = 0.5f * (1.0f - cosf(phase));                                                                                               
    hannWindowSum_ += hannWindow_[i];                                                                                                          
  }
}
/*
#==============================================================================================================================================================================================================================
#       Calcula la temporización del bloque y analiza los tres ejes.                                                                                                                                                          #                                                      
#                                                                                                                                                                                                                             # 
#   Fórmulas:                                                                                                                                                                                                                 #           
#     Fs = (N - 1) * 10^6 / elapsedUs                                                                                                                                                                                         #
#     Δf = Fs / N                                                                                                                                                                                                             #
#     TimestampDuration = (N - 1) / Fs                                                                                                                                                                                        #
#     eje_dominante = argmax(axis.peakAmplitudeRmsG)                                                                                                                                                                          #
#                                                                                                                                                                                                                             #                                                                  
#==============================================================================================================================================================================================================================
*/
void VibrationAnalyzer::analyzeBlock(uint32_t timestampMs, VibrationReport &report){
  report = VibrationReport{};                                                                                                                
  const uint32_t elapsedUs = lastSampleUs_ - firstSampleUs_;                                                                                  

  float effectiveSampleRateHz = Config::NOMINAL_SAMPLE_RATE_HZ;                                                                                
  if (elapsedUs > 0) {                                                                                                                        
    effectiveSampleRateHz =
        (Config::FFT_SIZE - 1) * 1000000.0f / static_cast<float>(elapsedUs);                                                                  
  }

  const float resolutionHz = effectiveSampleRateHz / Config::FFT_SIZE;                                                                         

  report.timestampMs = timestampMs;
  report.analysisId = ++analysisId_;
  report.effectiveSampleRateHz = effectiveSampleRateHz;
  report.resolutionHz = resolutionHz;
  report.blockDurationSeconds = (Config::FFT_SIZE - 1) / effectiveSampleRateHz;                                                                
  
  report.x = analyzeAxis(samplesX_, 'X', effectiveSampleRateHz);
  report.y = analyzeAxis(samplesY_, 'Y', effectiveSampleRateHz);
  report.z = analyzeAxis(samplesZ_, 'Z', effectiveSampleRateHz);

  const AxisVibrationResult *dominant = &report.x;

  if(report.y.peakAmplitudeRmsG > dominant->peakAmplitudeRmsG){dominant = &report.y;}
  if(report.z.peakAmplitudeRmsG > dominant->peakAmplitudeRmsG){dominant = &report.z;}

  report.dominantAxis = dominant->axis;
  report.dominantFrequencyHz = dominant->peakFrequencyHz;
  report.dominantAmplitudeRmsG = dominant->peakAmplitudeRmsG;
}
/*
#==============================================================================================================================================================================================================================
#       Calcula métricas temporales y espectrales de un eje.                                                                                                                                                                  #                                                      
#   1. Conversión de LSB a g y media                                                                                                                                                                                          #                                 
#   2. Eliminación de componente continua.                                                                                                                                                                                    #                                       
#   3. Métricas temporales.                                                                                                                                                                                                   #                       
#   4. Aplicación de ventanas y FFT.                                                                                                                                                                                          #                                       
#   5. Selección e interpolación del pico.                                                                                                                                                                                    #                                       
#                                                                                                                                                                                                                             # 
#   Donde se utiliza:                                                                                                                                                                                                         #                 
#   -  Conversión de LSB a g y media ->                                                                                                                                                                                       #                                     
#            x_g[n] = raw[n] * K  mean = (1 / N) * sum(x_g[n])                                                                                                                                                                #                                                         
#   -  Eliminación de componente continua ->                                                                                                                                                                                  #                                         
#            x_ac[n] = x_g[n] - mean                                                                                                                                                                                          #                               
#   -  Métricas temporales ->                                                                                                                                                                                                 #                           
#            RMS = sqrt((1/N) * sum(x_ac[n]^2))                                                                                                                                                                               #                                             
#            P2P = max(x_ac) - min(x_ac)                                                                                                                                                                                      #                                   
#            CF = max(abs(x_ac[n])) / RMS                                                                                                                                                                                     #                                     
#    -  Aplicación de ventana y FFT ->                                                                                                                                                                                        #                                     
#            Transformada discreta calculada por la FFT representa:                                                                                                                                                           #                                                               
#              x[k] = sum(x_windowed[n] * exp(-j*2*pi*k*n/N))                                                                                                                                                                 #                                                         
#            Resolución:                                                                                                                                                                                                      #                   
#              deltaF = FS / N                                                                                                                                                                                                #                         
#            Frecuencia del bin k:                                                                                                                                                                                            #                             
#              f[k] = k * deltaF                                                                                                                                                                                              #                           
#    -  Selección e interpolación del pico ->                                                                                                                                                                                 #                                         
#            kMin = ceil(fMin / deltaF)                                                                                                                                                                                       #                                   
#            kMax = floor(fMax / deltaF)                                                                                                                                                                                      #                                   
#            Posterior selección del bin con mayor amplitud RMS y se apl-                                                                                                                                                     #                                                                      
#            ca interpolación parabólica usando el bin central y vecinos.                                                                                                                                                     #                                                                 
#            Delta:                                                                                                                                                                                                           #               
#              delta = 0.5 * (left - right) / (left  - 2*center + right)                                                                                                                                                      #                                                                   
#            Frecuencia interpolada:                                                                                                                                                                                          #                               
#              fPeak = (peakBin + delta) * deltaF                                                                                                                                                                             #                                             
#            Amplitud interpolada:                                                                                                                                                                                            #                             
#              A_peak = center - 0.25 * (left - right) * delta                                                                                                                                                                #                                                                                                     
==============================================================================================================================================================================================================================
*/
AxisVibrationResult VibrationAnalyzer::analyzeAxis(const int16_t *samples, char axis,float effectiveSampleRateHz){
  AxisVibrationResult result;

  result.axis = axis;
  double sumG = 0.0;
  
  for(uint16_t i = 0; i< Config::FFT_SIZE; i++){
    sumG += samples[i] * Config::G_PER_LSB;
  }

  result.meanG = static_cast<float>(sumG / Config::FFT_SIZE);
  double sumSquares = 0.0;

  float minAcG = INFINITY;
  float maxAcG = -INFINITY;
  float absolutePeakG = 0.0f;

  for(uint16_t i = 0; i < Config::FFT_SIZE; i++){
    const float sampleG = samples[i] * Config::G_PER_LSB;                                 
    const float acG = sampleG - result.meanG;                                             

    sumSquares += static_cast<double>(acG) * acG;                                         
​
​

    if (acG < minAcG){ minAcG = acG;}
    if (acG > maxAcG){ maxAcG = acG;}

    const float absG = fabsf(acG);

    if (absG > absolutePeakG){ absolutePeakG = absG;}

    fftInput_[i] = acG * hannWindow_[i];

  }

  result.rmsG = sqrtf(static_cast<float>(sumSquares / Config::FFT_SIZE ));
  result.peakToPeakG = maxAcG - minAcG;                                                   
  result.crestFactor = result.rmsG > 0.0f ? absolutePeakG / result.rmsG : 0.0f;           
  fft_execute(fftPlan_);

  const float resolutionHz = effectiveSampleRateHz / Config::FFT_SIZE;
  
  uint16_t firstBin = static_cast<uint16_t>(ceilf(Config::MIN_ANALYSIS_FREQ_HZ / resolutionHz));
  uint16_t lastBin = static_cast<uint16_t>(floorf(Config::MAX_ANALYSIS_FREQ_HZ / resolutionHz));

  if(firstBin < 1){firstBin = 1;}

  const uint16_t highestRealBin = Config::FFT_SIZE / 2 - 1;

  if(lastBin > highestRealBin){lastBin = highestRealBin;}
  if(lastBin < firstBin){return result;}
  uint16_t peakBin = firstBin;
  
  float peakAmplitudeRmsG = getBinAmplitudeRmsG(firstBin);

  for(uint16_t bin = firstBin+1; bin <= lastBin; bin++){
    
    const float amplitudeRmsG = getBinAmplitudeRmsG(bin);

    if(amplitudeRmsG > peakAmplitudeRmsG){
      peakAmplitudeRmsG = amplitudeRmsG;
      peakBin = bin;
    }
  }

  float interpolatedBin = peakBin;
  float interpolatedAmplitudeRmsG = peakAmplitudeRmsG;

  if(peakBin > firstBin && peakBin < lastBin){

    const float left = getBinAmplitudeRmsG(peakBin - 1);
    const float center = peakAmplitudeRmsG;
    const float right = getBinAmplitudeRmsG(peakBin + 1);

    const float denominator = left - 2.0f * center + right;

    if(fabsf(denominator) > 1.0e-12f){
      float delta = 0.5f * (left - right) / denominator;

      if(delta > 0.5f){delta = 0.5f;}
      else if (delta < -0.5f){delta = -0.5f;}

      interpolatedBin = peakBin + delta;
      interpolatedAmplitudeRmsG = center - 0.25f * (left - right) * delta;

      if(interpolatedAmplitudeRmsG < 0.0f){interpolatedAmplitudeRmsG = 0.0f;}
    }
  }

  result.peakFrequencyHz = interpolatedBin * resolutionHz;
  result.peakAmplitudeRmsG = interpolatedAmplitudeRmsG;

  return result;

}

/*
#==============================================================================================================================================================================================================================
#       Obtiene la amplitud RMS de un bin del espectro.                                                                                                                                                                       #                                                                                                            
#                                                                                                                                                                                                                             #                                                        
#   Donde:                                                                                                                                                                                                                    #                                                              
#     - Parte real e imaginaria:                                                                                                                                                                                              #              
#        |realPart = fftOutput_[2 * bin];                                                                                                                                                                                     #                                                                                      
#         imagPart = fftOutput_[2 * bin + 1];                                                                                                                                                                                 #                          
#     - Magnitud compleja:                                                                                                                                                                                                    #        
#         magnitude = hypotf(realPart,imagPart) = sqrt(realPart^2 + imagPart^2)                                                                                                                                               #                                                            
#     - Amplitud de pico:                                                                                                                                                                                                     #      
#         amplitudePeakG = 2 * magnitude / hannWindowSum_ = 2 *|x[k]| / hannWindowSum_                                                                                                                                        #                                                                    
#     - Amplitud RMS:                                                                                                                                                                                                         #  
#         amplitudeRmsG = amplitudePeakG / sqrt(2)                                                                                                                                                                            #                                
#==============================================================================================================================================================================================================================
*/

float VibrationAnalyzer::getBinAmplitudeRmsG(uint16_t bin) const {

    if ( bin == 0 || bin >= Config::FFT_SIZE / 2 || hannWindowSum_ <= 0.0f ){ return 0.0f; }

    const float realPart = fftOutput_[ 2 * bin ];
    const float imagPart = fftOutput_[ 2 * bin + 1 ];
    const float magnitude = hypotf( realPart , imagPart );                                                            

    const float amplitudePeakG = 2.0f * magnitude / hannWindowSum_;                                                   

    return amplitudePeakG * INV_SQRT_2_F;
  
}