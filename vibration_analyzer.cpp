#include "vibration_analyzer.h"
#include <math.h>

namespace 
{
constexpr float TWO_PI_F = 6.28318530717958647692f;                                         // 2pi ,  para calcular la ventana Hann.
constexpr float INV_SQRT_2_F = 0.70710678118654752440f;                                     // 1 / √2 ,  para comvertir amplitud de pico a amplitud RMS en una señal sinusoidal.
}  

VibrationAnalyzer::~VibrationAnalyzer(){
  if(fftPlan_ !=nullptr){
    fft_destroy(fftPlan_);
    fftPlan_ = nullptr;
  }
}


bool VibrationAnalyzer::begin() {
  ready_ = false;

  sampleIndex_ = 0;
  firstSampleUs_ = 0;
  lastSampleUs = 0;
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

void VibrationAnalyzer::initializeHannWindow() {                                                                                                           
  hannWindowSum_ = 0.0f;                                                                                                                       // Se reinicia la suma cada vez que se inicializa la ventana

  for (uint16_t i = 0; i < Config::FFT_SIZE; i++) {                                                                                             // Se calculan 256 coeficientes:
    const float phase = TWO_PI_F * static_cast<float>(i) / static_cast<float>(Config::FFT_SIZE - 1);                                            // phase = [ 0 , 2pi ]
    hannWindow_[i] = 0.5f * (1.0f - cosf(phase));                                                                                               //  coeficiente_hann_i = 0.5 * (1-cos(phase_i))
    hannWindowSum_ += hannWindow_[i];                                                                                                           // acumula la suma de los coeficientes. Utilizado posteriormente para compensar la atenuación de amplitud.
  }
}

void VibrationAnalyzer::analyzeBlock(uint32_t timestampMs, VibrationReport &report){
  report = VibrationReport{};                                                                                                                 // Limpiar restos de datos recibidos.
  const uint32_t elapsedUs = lastSampleUs_ - firstSampleUs_;                                                                                  // Tiempo transcurrido entre primera y última muestra. Hay 256 muestras y 255 intervalos entre ellas.

  float effectiveSampleRateHz = Config::NOMINAL_SAMPLE_RATE_HZ;                                                                               // Por eso effectiveSampleRateHz = (FFT_SIZE - 1) * 1000000.0f / elapsedUs; 
  if (elapsedUs > 0) {                                                                                                                        // -> Fs = número de intervalos / tiempo. Se multiplica por 1M porque elapsed US está en ms.
    effectiveSampleRateHz =
        (Config::FFT_SIZE - 1) * 1000000.0f / static_cast<float>(elapsedUs);                                                                  // Entre N muestras existen N - 1 intervalos. F_S = (N-1) / Δt -> como elapsedUs está en μs​ ->  F_s = ((N - 1) * 10^6) / Δtμs​
  }

  const float resolutionHz = effectiveSampleRateHz / Config::FFT_SIZE;                                                                         // Resolución efectiva. Se recalcula con la frecuencia medida real. Si Fs efectiva = 99.88Hz entonces Δf = 99,88 / 256 ≈ 0,39016 Hz

  report.timestampMs = timestampMs;
  report.analysisId = ++analysisId_;
  report.effectiveSampleRateHz = effectiveSampleRateHz;
  report.resolutionHz = resolutionHz;
  report.blockDurationSeconds = (Config::FFT_SIZE - 1) / effectiveSampleRateHz;                                                                // Con N = 256 ,  F_s = 100Hz -> T_marcas = ((N-1) ( F_s )) = 2.55
  
  report.x = analyzeAxis(samplesX_, 'X', effectiveSampleRateHz);
  report.y = analyzeAxis(samplesY_, 'Y', effectiveSampleRateHz);
  report.z = analyzeAxis(samplesZ_, 'Z', effectiveSampleRateHz);

  const AxisVibrationResult *dominant = &report.x;

  if(report.y.peakAmplitudeRmsG > dominant->peakAmplitudeRmsG){dominant = &report.y;}
  if(report.z.peakAmplitudeRmsG > dominant->peakAmplitudeRmsG){dominant = &report.z;}

  report.dominantAxis = dominant-> axis;
  report.dominantFrequencyHz = dominant->peakFrequencyHz;
  report.dominantAmplitudeRmsG = dominant->peakAmplitudeRmsG;
}

AxisVibrationResult VibrationAnalyzer::analyzeAxis(const int16_t *samples, char axis,float effectiveSampleRateHz){
  AxisVibrationResult result;

  result.axis = axis;

  // ----------------------------------------------------------
  // 1. Calcular la media: gravedad, inclinación y offset DC.
  // ----------------------------------------------------------
  double sumG = 0.0;
  
  for(uint16_t i = 0; i< Config::FFT_SIZE; i++){
    sumG += samples[i] * Config::G_PER_LSB;
  }

  result.meanG = static_cast<float>(sumG / Config::FFT_SIZE);

  // ----------------------------------------------------------
  // 2. Métricas temporales y eliminación de DC
  // ----------------------------------------------------------

  double sumSquares = 0.0;

  float minAcG = INFINITY;
  float maxAcG = -INFINITY;
  float absolutePeakG = 0.0f;

  for(uint16_t i = 0; i < Config::FFT_SIZE; i++){
    const float sampleG = samples[i] * Config::G_PER_LSB;                                 //Conversión de LSB a G -> a_g​[n]=r[n]⋅K | r[n] es la lectura cruda y K = 0.0039g/LSB
    const float acG = sampleG - result.meanG;                                             // Eliminación de componente continua Media x = (1/N) * ​(n=0)(N-1)∑​x[n] -> señal centrada x_ac[n] = x[n] - x

    sumSquares += static_cast<double>(acG) * acG;                                         // x_RMS​= sqrt( (1/N) * (n=0)(N-1)∑x_ac[n]^2 )
​
​

    if (acG < minAcG){ minAcG = acG;}
    if (acG > maxAcG){ maxAcG = acG;}

    const float absG = fabs(acG);

    if (absG > absolutePeakG){ absolutePeakG = absG;}

    fftInput_[i] = acG * hannWindow_[i];

  }

  result.rmsG = sqrtf(static_cast<float>(sumSquares / Config::FFT_SIZE ));
  result.peakToPeakG = maxAcG - minAcG;                                                   // x_p-p = x_max - x_min -> exclusión total observada
  result.crestFactor = result.rmsG > 0.0f ? absolutePeakG / result.rmsG : 0.0f;           // CF = max(abs(x_ac[n]))/x_rms

  // ----------------------------------------------------------
  // 3. FFT
  // ----------------------------------------------------------

  fft_execute(fftPlan_),

  const float resolutionHz = effectiveSampleRateHz / Config::FFT_SIZE;
  
  uint16_t firstBin = static_cast<uint16_t>(ceilf(Config::MIN_ANALYSIS_FREQ_HZ / resolutionHz));
  uint16_t lastBin = static_cast<uint16_t>(floorf(Config::MAX_ANALYSIS_FREQ_HZ / resolutionHz));

  if(firstBin < 1){firstBin = 1;}

  const uint16_t highestRealBin = Config::FFT_SIZE / 2 - 1;

  if(lastBin > highestRealBin){lastBin = highestRealBin;}
  if(lastBin < firstBin){return result;}

  // ----------------------------------------------------------
  // 4. Buscar pico dominante
  // ----------------------------------------------------------

  uint16_t peakBin = firstBin;
  
  float peakAmplitudeRmsG = getBinAmplitudeRmsG(firstBin);

  for(uint16_t bin = firstBin+1; bin <= lastBin; bin++){
    
    const float amplitudeRmsG = getBinAmplitudeRmsG(bin);

    if(amplitudeRmsG > peakAmplitudeRmsG){
      peakAmplitudeRmsG = amplitudeRmsG;
      peakBin = bin;
    }
  }

  // ----------------------------------------------------------
  // 5. Interpolación parabólica
  // ----------------------------------------------------------
  
  float interpolatedBin = peakBin;
  float interpolatedAmplitudeRmsG = peakAmplitudeRmsG;

  if(peakBin > firstBin && peakBin < lastBin){

    const float left = getBinAmplitudeRmsG(peakBin - 1);
    const float center = peakAmplitudeRmsG;
    const float right = getBinAmplitudeRmsG(peakBin + 1);

    const float denominator = left - 2.0f * center + right;

    if(fabs(denominator) > 1.0e-12f){
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

float VibrationAnalyzer::getBinAmplitudeRmsG(uint16_t bin) const {

    if ( bin == 0 || bin >= Config::FFT_SIZE / 2 || hannWindowSum_ <= 0.0f ){ return 0.0f; }

    const float realPart = fftOutput_[ 2 * bin ];
    const float imagPart = fftOutput_[ 2 * bin + 1 ];
    const float magnitude = hypotf( realPart , imagPart );                                                            // |X[k]| = sqrt(parte_real(X[k])^2 + parte_imaginaria(X[k])^2)

    const float amplitudePeakG = 2.0f * magnitude / hannWindowSum_;                                                   // A_pico = (2|X[k]|) / sum(w[n]) -> A_rms = A_pico / sqrt(2)

    return amplitudePeakG * INV_SQRT_2_F;
  
}