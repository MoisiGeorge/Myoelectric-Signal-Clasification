#include <EloquentTinyML.h>
#include "gesture_model.h"
#include <TimerOne.h>
#include "arduinoFFT.h"
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>

//ML stuff
#define NUMBER_OF_INPUTS 4608
#define NUMBER_OF_OUTPUTS 16
// in future projects you may need to tweek this value: it's a trial and error process
#define TENSOR_ARENA_SIZE 120*1024

Eloquent::TinyML::TfLite<NUMBER_OF_INPUTS, NUMBER_OF_OUTPUTS, TENSOR_ARENA_SIZE> ml;

EXTMEM unsigned char model[d_model_len] DATA_ALIGN_ATTRIBUTE;

const char movement[16][10] = {"REST", "LF FLX", "RF FLX", "MF FLX", "IF FLX", "EXT", "FLX", "PRON", "SUPIN", "UL DE", "RAD DE", "LF EXT", "RF EXT", "MF EXT", "IF EXT", "ABDUC"};

//data colection stuff:
volatile double signalVec[4][400];
volatile int signalIndex = 0;
volatile bool specGen = 0;
volatile bool startInf = 0;
bool buttonState = 0;
volatile long int startTime;
volatile double sigEng = 0;
volatile double sigSum = 0;

//SpecGen stuff:
const int sigNum = 4;
arduinoFFT FFT = arduinoFFT();
const uint16_t samples = 128; //This value MUST ALWAYS be a power of 2
const double signalFrequency = 500;
const double samplingFrequency = 2000;
const uint8_t amplitude = 1;
const uint16_t outLength = 64;
const uint16_t outHeight = 18;

int heightIndex = 0;
int endPos = 128;
int startPos = 0;
const int stepPos = 16;

double vReal[samples];
double vImag[samples];
float specImage[sigNum * outLength * outHeight];

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

//Display stuff
#define TFT_CS         4
#define TFT_RST        3
#define TFT_DC         2

// For 1.14", 1.3", 1.54", and 2.0" TFT with ST7789:
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

void setup()   {
  Timer1.initialize(500);
//  Serial.begin(38400);
  pinMode(A11,INPUT_PULLDOWN);
  pinMode(A10,INPUT_PULLDOWN);
  
  memcpy((void *)model, (void *)d_model, d_model_len * sizeof(unsigned char));
  ml.begin(model);

  tft.init(135, 240);
  tft.fillScreen(ST77XX_BLACK);
  tft.invertDisplay(true);
//  Serial.println("initialization done.");
  delay(300);
  startTime = millis();
}

void readSignals(void){
//  Serial.println("Entered Interupt");
  signalVec[0][signalIndex] = ( (double) analogRead(A8) / 1023.0 ) * 3.3;
  signalVec[1][signalIndex] = ( (double) analogRead(A6) / 1023.0 ) * 3.3;
  signalVec[2][signalIndex] = ( (double) analogRead(A4) / 1023.0 ) * 3.3;
  signalVec[3][signalIndex] = ( (double) analogRead(A3) / 1023.0 ) * 3.3;
  
  if(signalIndex <= 40){
    sigSum  = sigSum + signalVec[0][signalIndex] + signalVec[1][signalIndex] + signalVec[2][signalIndex] + signalVec[3][signalIndex]; }
  
  signalIndex = signalIndex + 1;

  if(signalIndex == 40){
    if((sigSum / signalIndex) <= sigEng * 1.2){
      signalIndex = 0;
      sigSum = 0; }
  }
  
  if((signalIndex >= 128) && (signalIndex % 16 == 0)){
    specGen =  1; }
  
  if(signalIndex == 400){
    signalIndex = 0;
    startTime = millis(); }
}

void restEnergy(void){
  signalVec[0][signalIndex] = ( (double) analogRead(A8) / 1023.0 ) * 3.3;
  signalVec[1][signalIndex] = ( (double) analogRead(A6) / 1023.0 ) * 3.3;
  signalVec[2][signalIndex] = ( (double) analogRead(A4) / 1023.0 ) * 3.3;
  signalVec[3][signalIndex] = ( (double) analogRead(A3) / 1023.0 ) * 3.3;
  
  sigSum  = sigSum + signalVec[0][signalIndex] + signalVec[1][signalIndex] + signalVec[2][signalIndex] + signalVec[3][signalIndex];
  
  signalIndex = signalIndex + 1;
  
  if(signalIndex >= 40){
    sigEng = sigSum / signalIndex;
    signalIndex = 0;
    sigSum = 0;
  }
}

void loop()     
{
  if( (digitalRead(A11) == 1) && (buttonState == 0)){
//    Serial.println("Entered Loop");
    signalIndex = 0;
    sigSum = 0;
    Timer1.attachInterrupt(readSignals);
    buttonState = 1;
    
  }

  if( (digitalRead(A10) == 1) && (buttonState == 0)){
//    Serial.println("Calcucating rest Energy");
    sigSum = 0;
    Timer1.attachInterrupt(restEnergy);
    delay(200);
    Timer1.detachInterrupt();
    tft.setTextWrap(false);
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(0, 30);
    tft.setTextColor(ST77XX_WHITE);
    tft.setTextSize(2);
    tft.println("Init done");
    tft.println(" ");
    tft.println("sigEng:");
    tft.println(sigEng);
  }
  
  while(buttonState){
    if(startInf){
      startInf = 0;
      float y_pred[NUMBER_OF_OUTPUTS] = { 0 };
      
      ml.predict(specImage, y_pred);
      long int elapsed = millis() - startTime;
      for (int i = 0; i < NUMBER_OF_OUTPUTS; i++) {
//        Serial.print(y_pred[i]);
//        Serial.print(i == (NUMBER_OF_OUTPUTS - 1) ? '\n' : ',');
      }
//      Serial.print("Predicted class is: ");
//      Serial.println(ml.probaToClass(y_pred));
//      
//      Serial.print("Elapsed Time: ");
//      Serial.println( elapsed );
      
      tft.setTextWrap(false);
      tft.fillScreen(ST77XX_BLACK);
      tft.setCursor(0, 30);
      tft.setTextColor(ST77XX_RED);
      tft.setTextSize(2);
      tft.println("Elapsed:");
      tft.println(" ");
      tft.print("  ");
      tft.setTextColor(ST77XX_WHITE);
      tft.print(elapsed);
      tft.println(" ms");
      tft.println(" ");
      tft.setTextColor(ST77XX_GREEN);
      tft.println("Miscare:");
      tft.setTextColor(ST77XX_WHITE);
      tft.println(" ");
      tft.setTextSize(3);
      tft.println(movement[ml.probaToClass(y_pred)]);
    }

    if(specGen){
      //Serial.println("Spectogram ...");
      if(heightIndex == 0){
        endPos = 128;
        startPos = 0; }
      
      for(int k = 0; k < sigNum; k++){
        for(int i = startPos; i < endPos; i++){
            vReal[i - startPos] = signalVec[k][i];
        }
        double cycles = (((samples-1) * signalFrequency) / samplingFrequency); //Number of signal cycles that the sampling will read
        for (int i = 0; i < samples; i++)
        {
          vImag[i] = 0.0; //Imaginary part must be zeroed in case of looping to avoid wrong calculations and overflows
        }
        FFT.Windowing(vReal, samples, FFT_WIN_TYP_HAMMING, FFT_FORWARD);  /* Weigh data */
          
        FFT.Compute(vReal, vImag, samples, FFT_FORWARD); /* Compute FFT */
        
        FFT.ComplexToMagnitude(vReal, vImag, samples); /* Compute magnitudes */

        for(int i = 0; i < outLength; i++){
          specImage[(i * outHeight) + heightIndex + (outHeight * outLength * k)] = vReal[i];
        }
      }
      
      heightIndex = heightIndex + 1;
      
      startPos = startPos + stepPos;
      endPos = endPos + stepPos;

      specGen = 0;
      
      if(heightIndex == 18){
        heightIndex = 0;
        startInf = 1;}
    }
      
  }
  
  delay(200);
}
