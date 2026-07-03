#include <Arduino.h>
#include "fft.h"

#include "measurement_types.h"


namespace {
  static const float TWO_PI_F = 6.28318530717958647692f;                                                                              // 2pi ,  para calcular la ventana Hann.
  static const float INV_SQRT_2_F = 0.70710678118654752440f;                                                                          // 1 / √2 ,  para comvertir amplitud de pico a amplitud RMS en una señal sinusoidal.

}

VibrationAnalyzer::~VibrationAnalyzer(){
  if(fftPlan_ !=nullptr){
    fft_destroy(fftPlan_);
    fftPlan_ = nullptr;
  }
}


bool VibrationAnalyzer::begin() {
  ready_ = false;

  sampleIndex = 0;
  firstSampleUs = 0;
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

  if(sampleIndex_ < Config:FFT_SIZE){ return false; }

  analyzeBlock(millis(),report);

  sampleIndex_ = 0;
  
  return true;
}

void VibrationAnalyzer::initializeHannWindow() {                                                                                                           
  hannWindowSum = 0.0f;                                                                                                                       // Se reinicia la suma cada vez que se inicializa la ventana

  for (uint16_t i = 0; i < FFT_SIZE; i++) {                                                                                                   // Se calculan 256 coeficientes:
    const float phase = TWO_PI_F * static_cast<float>(i) / static_cast<float>(Config::FFT_SIZE - 1);                                                                                        // phase = [ 0 , 2pi ]
    hannWindow[i] = 0.5f * (1.0f - cosf(phase));                                                                                              // coeficiente_hann_i = 0.5 * (1-cos(phase_i))
    hannWindowSum += hannWindow[i];                                                                                                           // acumula la suma de los coeficientes. Utilizado posteriormente para compensar la atenuación de amplitud.
  }
}

void VibrationAnalyzer::analyzeBlock(uint32_t timestampMs, VibrationReport &report){
  report = VibrationReport{};                                                                                                                 // Limpiar restos de datos recibidos.
  const uint32_t elapsedUs = lastSampleUs_ - firstSampleUs_;                                                                                  // Tiempo transcurrido entre primera y última muestra. Hay 256 muestras y 255 intervalos entre ellas.

  float effectiveSampleRateHz = Config::NOMINAL_SAMPLE_RATE_HZ;                                                                               // Por eso effectiveSampleRateHz = (FFT_SIZE - 1) * 1000000.0f / elapsedUs; 
  if (elapsedUs > 0) {                                                                                                                        // -> Fs = número de intervalos / tiempo. Se multiplica por 1M porque elapsed US está en ms.
    effectiveSampleRateHz =
        (Config::FFT_SIZE - 1) * 1000000.0f / static_cast<float>(elapsedUs);
  }

  const float resolutionHz = effectiveSampleRateHz / Config::FFT_SIZE;                                                                         // Resolución efectiva. Se recalcula con la frecuencia medida real. Si Fs efectiva = 99.88Hz entonces Δf = 99,88 / 256 ≈ 0,39016 Hz

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
  
  for(uint16_t i = 0; i< Config:FFT_SIZE; i++){
    sumG += swamples[i] * config::G_PER_LSB;
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
    const float sampleG = samples[i] * Config::G_PER_LSB;
    const float acG = sampleG - result.meanG
    
    sumSquares += static_cast<double>(acG) * acG:

    if (acG < minAcG){ minAcG = acG;}
    if (acG > maxAcG){ maxAcG = acG;}

    const float absG = fabs(acG)

    if (absG > absolutePeakG){ absolutePeakG = absG;}

    fftInput[i] = acG * hannWindow_[i];

  }

  result.rmsG = sqrtf(static_cast<float>(sumSquares / Config::FFT_SIZE ));
  result.peakToPeakG = maxAcG - minAcG;
  result.crestFactor = result.rmsG > 0.0f ? absolutePeakG / result.rmsG : 0.0f;

  // ----------------------------------------------------------
  // 3. FFT
  // ----------------------------------------------------------

  fft_execute(fftPlan_)

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

      if(delta > 0.5f){delta = 0.5f}
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

    if ( bin == 0 || bin >= Config::FFT_SIZE / 2 || hannWindowSum_ <= 0.0f ){ return 0.0f }

    const float realPart = fftOutput_[ 2 * bin ];
    const float imagPart = fftOutput_[ 2 * bin + 1 ];
    const float magnitude = hypotf( realPart , imagPart );

    const float amplitudePeakG = 2.0f * magnitude / hannWindowSum_;

    return ampltudePeakG * INV_SQRT_2_F;
  
}




bool initFFT() {                                                                                                                               
  initHannWindow();                                                                                                                           // Primero se genera la ventana Hann

                                                                                                                                              // Los buffers se suministran explícitamente para que fft_init no los reserve.
                                                                                                                                              // El plan y los twiddle factors sí se reservan una sola vez durante setup().
  fftPlan = fft_init(
      FFT_SIZE,                                                                                                                               // FFT_SIZE = 256 muestras
      FFT_REAL,                                                                                                                               // FFT_REAL -> entrada en valores reales, no complejos 
      FFT_FORWARD,                                                                                                                            // FFT_FORWARD -> se quiere transformar tiempo a frecuencia. No inversa.
      fftInput,                                                                                                                               // Buffer de entrada 
      fftOutput                                                                                                                               // Buffer de salida
  );

  return fftPlan != nullptr;
}



void storeSample(int16_t x, int16_t y, int16_t z, uint32_t sampleTimeUs) {
  if (fftSampleIndex == 0) {
    fftFirstSampleUs = sampleTimeUs;                                                                                                            // Si es la primera muestra del bloque se guarda el tiempo inicial
  }

  fftSamplesX[fftSampleIndex] = x;                                                                                                              // Se guardan los tres ejes en la misma posición.
  fftSamplesY[fftSampleIndex] = y;                                                                                                              // Esto asegura que X[n],Y[n],Z[n] siempre correspondan al mismo instante para el mismo valor de n en los tres vectores.
  fftSamplesZ[fftSampleIndex] = z;
  fftLastSampleUs = sampleTimeUs;                                                                                                               // Marca de la última muestra

  fftSampleIndex++;                                                                                                                             // Avanza a la siguiente posición de los buffers

  if (fftSampleIndex >= FFT_SIZE) {                                                                                                             // Si el índice ha llegado al final del tamaño del bloque de muestreo ( 256 ) -> Se aaliza el bloque -> Se reinicia el índice -> El siguiente dato será la primera muestra del siguiente bloque
    analyzeFFTBlock(millis());
    fftSampleIndex = 0;
  }
}

// ============================================================
// ANÁLISIS DE VIBRACIONES
// ============================================================
void VibrationAnalyzer::analyzeFFTBlock(uint32_t timestampMs,VibrationReport &report) {                                                         // Función de análisis del bloque de 256 muestras X,Y,Z para la FFT
  report = VibrationReport{};

  const uint32_t elapsedUs = fftLastSampleUs - fftFirstSampleUs;                                                                                // Tiempo transcurrido entre primera y última muestra. Hay 256 muestras y 255 intervalos entre ellas.
                                                                                                                                                // muestra 0 - intervalo - muestra 1 - intervalo - ... - muestra 255  -> 
  float effectiveSampleRateHz = NOMINAL_SAMPLE_RATE_HZ;                                                                                         // Por eso effectiveSampleRateHz = (FFT_SIZE - 1) * 1000000.0f / elapsedUs; 
  if (elapsedUs > 0) {                                                                                                                          // -> Fs = número de intervalos / tiempo. Se multiplica por 1M porque elapsed US está en ms.
    effectiveSampleRateHz =
        (FFT_SIZE - 1) * 1000000.0f / elapsedUs;
  }

  const float resolutionHz = effectiveSampleRateHz / FFT_SIZE;                                                                                   // Resolución efectiva. Se recalcula con la frecuencia medida real. Si Fs efectiva = 99.88Hz entonces Δf = 99,88 / 256 ≈ 0,39016 Hz

  const VibrationResult xResult =                                                                                                                // Análisis eje X
      analyzeAxis(fftSamplesX, 'X', effectiveSampleRateHz);
  const VibrationResult yResult =
      analyzeAxis(fftSamplesY, 'Y', effectiveSampleRateHz);                                                                                      // Análisis eje Y
  const VibrationResult zResult =
      analyzeAxis(fftSamplesZ, 'Z', effectiveSampleRateHz);                                                                                      // Análisis eje Z
                                                                                                                                                 // Se reutilizan el mismo fftInput,fftOutput y fftPlan
  const VibrationResult *dominant = &xResult;                                                                                                    // Elección de eje dominante. Inicialmente se supone que X domina
  if (yResult.peakAmplitudeRmsG > dominant->peakAmplitudeRmsG) {                                                                                 // Inicialmente se supone que X domina.
    dominant = &yResult;                                                                                                                         // Luego -> si Y tiene un pico espectral mayor pasa a ser dominante
  }
  if (zResult.peakAmplitudeRmsG > dominant->peakAmplitudeRmsG) {
    dominant = &zResult;                                                                                                                         // Si Z tiene un pico espectral mayor pasa a ser dominante
  }                                                                                                                                              // El eje dominante es el que tiene la lína espectral individual más intensa. Se basa solamente en peakAmplitudeRMSG

  fftAnalysisId++;                                                                                                                               // Se incrementa el número de análisis realizados.

  Serial.println();                                                                                                                              // Print de resultados
  Serial.printf(
      "FFT #%lu | Fs=%.3f Hz | dF=%.4f Hz | bloque=%.3f s\n",
      static_cast<unsigned long>(fftAnalysisId),
      effectiveSampleRateHz,
      resolutionHz,
      (FFT_SIZE - 1) / effectiveSampleRateHz
  );

  Serial.printf(
      "  X: RMS=%.5f g  P-P=%.5f g  Cresta=%.2f"
      "  Pico=%.3f Hz @ %.5f g RMS\n",
      xResult.rmsG,
      xResult.peakToPeakG,
      xResult.crestFactor,
      xResult.peakFrequencyHz,
      xResult.peakAmplitudeRmsG
  );

  Serial.printf(
      "  Y: RMS=%.5f g  P-P=%.5f g  Cresta=%.2f"
      "  Pico=%.3f Hz @ %.5f g RMS\n",
      yResult.rmsG,
      yResult.peakToPeakG,
      yResult.crestFactor,
      yResult.peakFrequencyHz,
      yResult.peakAmplitudeRmsG
  );

  Serial.printf(
      "  Z: RMS=%.5f g  P-P=%.5f g  Cresta=%.2f"
      "  Pico=%.3f Hz @ %.5f g RMS\n",
      zResult.rmsG,
      zResult.peakToPeakG,
      zResult.crestFactor,
      zResult.peakFrequencyHz,
      zResult.peakAmplitudeRmsG
  );

  Serial.printf(
      "  DOMINANTE: eje %c, %.3f Hz, %.5f g RMS"
      " | overruns total=%lu | heap=%u\n\n",
      dominant->axis,
      dominant->peakFrequencyHz,
      dominant->peakAmplitudeRmsG,
      static_cast<unsigned long>(overrunTotal),
      ESP.getFreeHeap()
  );

  if (sdOK) {
    if (!appendVibrationCSV(
            timestampMs,
            effectiveSampleRateHz,
            resolutionHz,
            xResult,
            yResult,
            zResult,
            *dominant
        )) {
      Serial.println("Error escribiendo vibration_fft.csv");
    }
  }
}

VibrationResult analyzeAxis(                                                                                                                     //                                               
    const int16_t *samples,
    char axis,
    float effectiveSampleRateHz
) {
  VibrationResult result{};                                                                                                                      // Crea resultado y lo inicializa a cero
  result.axis = axis;                                                                                                                            // Guarda el eje actual en el objeto

  // ----------------------------------------------------------
  // 1. Calcular la media: gravedad, inclinación y offset DC.
  // ----------------------------------------------------------
  double sumG = 0.0;                                                                                                                              // Calcula media del bloque.
  for (uint16_t i = 0; i < FFT_SIZE; i++) {
    sumG += samples[i] * G_PER_LSB;                                                                                                               // Convierte cada muestra a g y la suma
  }
  result.meanG = static_cast<float>(sumG / FFT_SIZE);                                                                                             // Calcula la componente media y la asigna

  // ----------------------------------------------------------
  // 2. Quitar la media y obtener métricas temporales.
  // ----------------------------------------------------------
  double sumSquares = 0.0;                                                                                                                         // 
  float minAcG = INFINITY;                                                                                                                         // Inicializa a infinito para garantizar que la primera muestra sustituirá ambos valores
  float maxAcG = -INFINITY;                                                                                                                        // 
  float absolutePeakG = 0.0f;                                                                                                                      //

  for (uint16_t i = 0; i < FFT_SIZE; i++) {                                                                                                        // Quitar componente continua 
    const float sampleG = samples[i] * G_PER_LSB;                                                                                                  // Aceleración total
    const float acG = sampleG - result.meanG;                                                                                                      // Parte variable alrededor de la memoria

    sumSquares += static_cast<double>(acG) * acG;                                                                                                  // Acumula cuadrado de cada muestra

    if (acG < minAcG) {
      minAcG = acG;                                                                                                                                // Busca el mínimo y el máximo
    }
    if (acG > maxAcG) {
      maxAcG = acG;
    }

    const float absG = fabsf(acG);                                                                                                                 // Obtiene el valor absoluto.
    if (absG > absolutePeakG) {
      absolutePeakG = absG;                                                                                                                        // Busca el mayor alejamiento respecto a cero
    }

    // --------------------------------------------------------
    // 3. Aplicar ventana Hann antes de la FFT.
    // --------------------------------------------------------
    fftInput[i] = acG * hannWindow[i];                                                                                                              // Aplica ventana Hann. Cada muestra centrada se multiplica por su coeficiente de ventana.
  }

  result.rmsG = sqrtf(static_cast<float>(sumSquares / FFT_SIZE));                                                                                   // RMS = √[(x₁² + x₂² + ... + xₙ²) / N]. Representa la energía global de la vibración en ese eje. 
  result.peakToPeakG = maxAcG - minAcG;                                                                                                             // Guarda rango de pico. Por ejemplo max = +0.8g , min = -0.6g -> P-P = 0.8 - (-0.6) = 0.14 g
  result.crestFactor =
      result.rmsG > 0.0f ? absolutePeakG / result.rmsG : 0.0f;                                                                                      //

  // ----------------------------------------------------------
  // 4. Transformar al dominio de frecuencia.
  // ----------------------------------------------------------
  fft_execute(fftPlan);                                                                                                                              // Computa la FFT. Transforma las 256 muestras de fftInput al dominio frecuencial y escribe el resultado en fftOutput

  const float resolutionHz = effectiveSampleRateHz / FFT_SIZE;                                                                                       // Conversión de límites de frecuencia a bins. Cada bin representa resolutionHz

  uint16_t firstBin = static_cast<uint16_t>(                                                                                                         // Se calcula el primer bin cuya frecuencia sea igual o superior a 1Hz
      ceilf(MIN_ANALYSIS_FREQ_HZ / resolutionHz)                                                                                                     // Ej: con resolutionHz = 0.390625Hz -> 1/ 0.390625 = 2,56 -> ceil(2.56) = 3 -> El primer bin analizado será el 3
  );
  uint16_t lastBin = static_cast<uint16_t>(                                                                                                          // Mismo proceso. Obtiene el último bin que no supera 45Hz.
      floorf(MAX_ANALYSIS_FREQ_HZ / resolutionHz)
  );

  if (firstBin < 1) {                                                                                                                                 // El bin 0  se excluye
    firstBin = 1;
  }

  const uint16_t highestRealBin = FFT_SIZE / 2 - 1;                                                                                                   // Con 256 muestras higestRealBin = 127. la mitad positiva de una FFt real hasta aproximadamente Nyquist.
  if (lastBin > highestRealBin) {                                                                                                                     // Misma comprobación de antes. Evita acceder fuera del rango válido.
    lastBin = highestRealBin;
  }

  if (lastBin <= firstBin) {                                                                        
    result.peakFrequencyHz = 0.0f;
    result.peakAmplitudeRmsG = 0.0f;
    return result;
  }

  // ----------------------------------------------------------
  // 5. Buscar la línea espectral de mayor amplitud.
  // ----------------------------------------------------------
  uint16_t peakBin = firstBin;                                                                                                                         // Buscar el pico máximo -> Se comienza suponiendo que el primer bin es el máximo.
  float peakAmplitudeRmsG = getBinAmplitudeRmsG(firstBin);

  for (uint16_t bin = firstBin + 1; bin <= lastBin; bin++) {                                                                                           // Se recorren todos los bins de la banda.
    const float amplitudeRmsG = getBinAmplitudeRmsG(bin);                                                                                              // Se calcula su amplitud

    if (amplitudeRmsG > peakAmplitudeRmsG) {                                                                                                           // Si aparece una amplitud mayor -> Se actualiza el máximo.
      peakAmplitudeRmsG = amplitudeRmsG;
      peakBin = bin;
    }
  }

  // ----------------------------------------------------------
  // 6. Interpolación parabólica para mejorar la estimación de
  //    frecuencia cuando el pico queda entre dos bins.
  // ----------------------------------------------------------
  float interpolatedBin = peakBin;                                                                                                                      // Suponiendo que la vibración real está en 24.8Hz pero los bins mas cercanos son 24.609Hz y 25.000Hz
  float interpolatedAmplitudeRmsG = peakAmplitudeRmsG;                                                                                                  // El pico se repartirá entre ambos

  if (peakBin > firstBin && peakBin < lastBin) {
    const float left   = getBinAmplitudeRmsG(peakBin - 1);                                                                                              // bin anterior
    const float center = peakAmplitudeRmsG;                                                                                                             // bin máximo
    const float right  = getBinAmplitudeRmsG(peakBin + 1);                                                                                              // bin posterior

    const float denominator = left - 2.0f * center + right;                                                                                             // curvatura total

    if (fabsf(denominator) > 1.0e-12f) {                                                                                                                //
      float delta = 0.5f * (left - right) / denominator;

      if (delta > 0.5f) {
        delta = 0.5f;
      } else if (delta < -0.5f) {
        delta = -0.5f;
      }

      interpolatedBin = peakBin + delta;                                                                                                                 // Obtiene una posición fraccionaria. peakBin=65 , delta=-0.3 -> binInterpolad= 63,7
      interpolatedAmplitudeRmsG =
          center - 0.25f * (left - right) * delta;
    }
  }

  result.peakFrequencyHz = interpolatedBin * resolutionHz;                                                                                               // Convierte la posición interpolada a Hercios. La interpolación es porque mejora la estimación de la señal.
  result.peakAmplitudeRmsG = interpolatedAmplitudeRmsG;                                                               

  return result;
}

float getBinAmplitudeRmsG(uint16_t bin) {                                                                                                                // Extraer la amplitud de un bin. Convierte un resultado complejo de la FFT en amplitud RMS expresada en g.
  if (bin == 0 || bin >= FFT_SIZE / 2) {
    return 0.0f;
  }

  // En la FFT real de esta biblioteca:
  // Re(X[k]) está en output[2*k] e Im(X[k]) en output[2*k+1].
  const float realPart = fftOutput[2 * bin];                                                                                                             // Parte real e imaginaria. La biblioteca almacena cada bin complejo como parte real y parte imaginaria. la posición depende del mundo del bin
  const float imagPart = fftOutput[2 * bin + 1];                                                                                                         // Para el bin 10 -> real = output[20] , imag = output[21]
  const float magnitude = hypotf(realPart, imagPart);                                                                                                    // Magnitud compleja: Sigue la fórmula magnitud = √(real² + imaginaria²)

  // Espectro unilateral corregido por la ganancia coherente de la ventana.
  const float amplitudePeakG = 2.0f * magnitude / hannWindowSum;                                                                                         // Espectro unilateral -> 2.0f * magnitude / hannWindowSum;. El 2 es porque solo se utiliza la mitad positiva del espectro.
  return amplitudePeakG * INV_SQRT_2_F;                                                                                                                  // Convierte a RMS y devuelve. RMS = pico / √2
}
