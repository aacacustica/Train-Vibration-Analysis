
#include <FS.h>
#include <SD.h>

#include "config.h"
#include "sd_logger.h"



SDLogger::SDLogger(SPIClass &spi) : spi_(spi){}

bool SDLogger::begin(){

  /*
    ===================================================================================================================================================                                                                                                                                                  
    #  Inicializa la tarjeta y prepara los archivos:                                                                                                  #                                              
    #                                                                                                                                                 #   
    #    1. Configura CS y llama a SD.begin() sobre el bus HSPI.                                                                                      #                                                          
    #    2. Comprueba que exista una tarjeta.                                                                                                         #                                      
    #    3. Informa del tipo y capacidad.                                                                                                             #                                  
    #    4. Crea adxl_average.csv si no existe.                                                                                                       #                                        
    #    5. Crea vibration_fft.csv si no existe.                                                                                                      #                                          
    #    6. Marca el logger como disponible únicamente si ambas                                                                                       #                                                        
    #       cabeceras han podido verificarse o crearse.                                                                                               #                                                
    ===================================================================================================================================================
*/

  ready_ = false;
  
  pinMode(Config::SD_CS,OUTPUT);

  digitalWrite(Config::SD_CS,HIGH);

  if(!SD.begin(Config::SD_CS,spi_)){
    Serial.println("Fallo al inicializar la SD por HSPI");
    return false;
  }

  const uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No hay tarjeta SD");
    return false;
  }

  Serial.print("Tipo de tarjeta");
  if(cardType == CARD_MMC){ Serial.println("MMC"); }
  else if(cardType == CARD_SD){ Serial.println("SDSC"); }
  else if(cardType == CARD_SDHC){ Serial.println("SDHC"); }
  else{ Serial.println("UNKNOWN"); }

  const uint64_t cardSizeMb = SD.cardSize() / (1024ULL * 1024ULL);

  Serial.printf("Tamaño SD: %llu MB\n", static_cast<unsigned long long>(cardSizeMb));

const bool averageHeaderOK =
    ensureCSVHeader(
        Config::AVERAGE_LOG_PATH,
        "timestamp_ms,"
        "x_avg_g,"
        "y_avg_g,"
        "z_avg_g,"
        "samples,"
        "overruns,"
        "heap_free"
    );

  const bool vibrationHeaderOK = 
    ensureCSVHeader(
        Config::VIBRATION_LOG_PATH,
        "timestamp_ms,"
        "analysis_id,"
        "fs_hz,"
        "resolution_hz,"
        "x_rms_g,"
        "x_p2p_g,"
        "x_crest,"
        "x_peak_hz,"
        "x_peak_rms_g,"
        "y_rms_g,"
        "y_p2p_g,"
        "y_crest,"
        "y_peak_hz,"
        "y_peak_rms_g,"
        "z_rms_g,"
        "z_p2p_g,"
        "z_crest,"
        "z_peak_hz,"
        "z_peak_rms_g,"
        "dominant_axis,"
        "dominant_peak_hz,"
        "dominant_peak_rms_g,"
        "overruns_total,"
        "heap_free"
    );

  ready_ = averageHeaderOK && vibrationHeaderOK;

  return ready_;
}

bool SDLogger::isReady() const { return ready_; }

bool SDLogger::appendAverage( const AverageResult &result){

  if(!ready_){ return false; }

  File file = SD.open(Config::AVERAGE_LOG_PATH,FILE_APPEND);

  if(!file) { return false; }

  file.print( result.timestampMs );
  file.print( ',' );
  file.print( result.xG, 6 );
  file.print( ',' );
  file.print( result.yG, 6 );
  file.print( ',' );
  file.print( result.zG, 6 );
  file.print( ',' );
  file.print( result.sampleCount );
  file.print( ',' );
  file.print( result.overruns );
  file.print( ',' );
  file.println( ESP.getFreeHeap() );
  file.close();

  return true;
}

bool SDLogger::appendVibration(const VibrationReport &report, uint32_t totalOverruns){

  /*
    ===================================================================================================================================================
    # Añade una fila con el resumen del último bloque FFT.                                                                                            #                                                      
    #                                                                                                                                                 #
    #  Para cada eje guarda:                                                                                                                          #                        
    #  - RMS total.                                                                                                                                   #              
    #  - Pico a pico.                                                                                                                                 #                
    #  - Factor de cresta.                                                                                                                            #                      
    #  - Frecuencia dominante.                                                                                                                        #                          
    #  - Amplitud RMS de la frecuencia dominante.                                                                                                     #                                            
    #                                                                                                                                                 #
    #  También guarda la frecuencia de muestreo efectiva, resolución                                                                                  #                                                                
    #  espectral, eje dominante, overruns y memoria libre.                                                                                            #                                                                                
    ===================================================================================================================================================
*/

  if(!ready_){ return false; }

  File file = SD.open( Config::VIBRATION_LOG_PATH, FILE_APPEND );

  if(!file){ return false; }

  file.print( report.timestampMs );
  file.print( ',' );
  file.print( report.analysisId );
  file.print( ',' );
  file.print( report.effectiveSampleRateHz,6);
  file.print( ',' );
  file.print( report.resolutionHz,6);
  file.print( ',' );

  file.print( report.x.rmsG, 6);
  file.print( ',' );
  file.print( report.x.peakToPeakG, 6);
  file.print( ',' );
  file.print( report.x.crestFactor, 4);
  file.print( ',' );
  file.print( report.x.peakFrequencyHz, 6);
  file.print( ',' );
  file.print( report.x.peakAmplitudeRmsG, 6);
  file.print( ',' );

  file.print( report.y.rmsG, 6);
  file.print( ',' );
  file.print( report.y.peakToPeakG, 6);
  file.print( ',' );
  file.print( report.y.crestFactor, 4);
  file.print( ',' );
  file.print( report.y.peakFrequencyHz, 6);
  file.print( ',' );
  file.print( report.y.peakAmplitudeRmsG, 6);
  file.print( ',' );

  file.print( report.z.rmsG, 6);
  file.print( ',' );
  file.print( report.z.peakToPeakG, 6);
  file.print( ',' );
  file.print( report.z.crestFactor, 4);
  file.print( ',' );
  file.print( report.z.peakFrequencyHz, 6);
  file.print( ',' );
  file.print( report.z.peakAmplitudeRmsG, 6);
  file.print( ',' );

  file.print( report.dominantAxis );
  file.print( ',' );
  file.print( report.dominantFrequencyHz, 6);
  file.print( ',' );
  file.print( report.dominantAmplitudeRmsG, 6);
  file.print( ',' );
  
  file.print( totalOverruns );
  file.print( ',' );
  file.println(ESP.getFreeHeap());

  file.close();

  return true;
}

void SDLogger::listRoot(){

  if(ready_){ listDirectory( "/",1 ); }

}

bool SDLogger::ensureCSVHeader( const char *path, const char *header){

  if(SD.exists(path)){ 
    Serial.printf("CSV existente: %s\n",path);
    return true;
  }

  File file = SD.open( path,FILE_WRITE );

  if( !file ){ 
    Serial.printf( "No se pudo crear: %s\n",path);
    return false;
  }

  file.println(header);
  file.close();

  Serial.printf( "CSV creado: %s\n",path );

  return true;
}

void SDLogger::listDirectory( const char *dirname, uint8_t levels){

  Serial.printf( "Directorio: %s\n",dirname );
  File root = SD.open( dirname );

  if( !root ){ Serial.println( "No se pudo abrir el directorio" ); return; }
  

  if( !root.isDirectory() ){ Serial.println( "La ruta no es un directorio" ); root.close(); return; }

  File file = root.openNextFile();

  while(file){

    if (file.isDirectory()){
  
      Serial.print(" DIR: ");
      Serial.print( file.name() );
      if( levels > 0 ){ listDirectory( file.path(), levels - 1 );}

    }else{
        Serial.print( " FILE: ");
        Serial.print( file.name() );

        Serial.print( " SIZE: ");
        Serial.print( file.size(); )
      }

      file.close();
      file = root.openNextFile();
  }

  root.close();

}

