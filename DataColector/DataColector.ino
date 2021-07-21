/* Serial Monitor Example, Teensyduino Tutorial #3
   http://www.pjrc.com/teensy/tutorial3.html

   After uploading this to your board, use Serial Monitor
   to view the message.  When Serial is selected from the
   Tools > USB Type menu, the correct serial port must be
   selected from the Tools > Serial Port AFTER Teensy is
   running this code.  Teensy only becomes a serial device
   while this code is running!  For non-Serial types,
   the Serial port is emulated, so no port needs to be
   selected.

   This example code is in the public domain.
*/
#include <TimerOne.h>
#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>
File myFile;
//StaticJsonDocument<200000> doc;
const int chipSelect = BUILTIN_SDCARD;
const int ledPin = 13;

volatile int signalVec[4][20000];
volatile int signalIndex = 0;
bool buttonState = 0;
long int startTime = millis();
long int elapsedTime = 0;
int label = 0;
int repeats = 0;
const int sigNum = 4;


void setup()   {
  Timer1.initialize(500);
  Serial.begin(38400);
  pinMode(A11,INPUT_PULLDOWN);
  pinMode(A10,INPUT_PULLDOWN);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  if (!SD.begin(chipSelect)) {
//    Serial.println("initialization failed!");
    return;
  }
//  Serial.println("initialization done.");
  delay(300);
 
}

void readSignals(void){
  //Serial.println("Entered Interupt");
  signalVec[0][signalIndex] = analogRead(A8);
  signalVec[1][signalIndex] = analogRead(A6);
  signalVec[2][signalIndex] = analogRead(A4);
  signalVec[3][signalIndex] = analogRead(A3);
  signalIndex = signalIndex + 1;
}

void loop()     
{
  if( digitalRead(A11) == 1){
//    Serial.println("Entered Loop");
    digitalWrite(ledPin, LOW);
    Timer1.attachInterrupt(readSignals);
    buttonState = 1;
    repeats += 1;
  }
  startTime = millis();
  
  while(buttonState && ((elapsedTime = (millis() - startTime)) < 10000)){
//    Serial.println(elapsedTime);
    delayMicroseconds(500);
  }
  if(buttonState == 1){
    buttonState = 0;
    digitalWrite(ledPin, HIGH);
    Timer1.detachInterrupt();
    WriteSig();
    
//    Serial.print("Label: ");
//    Serial.println(label);
//    Serial.println("--------------------------------Signal Content----------------------------------");
//    for(int i = 0; i <= signalIndex; i++){
//      Serial.print(signalString[0][i]);
//      Serial.print("  ");
//      Serial.print(signalString[1][i]);
//      Serial.print("  ");
//      Serial.print(signalString[2][i]);
//      Serial.print("  ");
//      Serial.print(signalString[3][i]);
//      Serial.println("");
//    }
    signalIndex = 0;
  }
  if(digitalRead(A10) == 1){
      label += 1;
      repeats = 0;
      delay(300);
      digitalWrite(ledPin, LOW);
      delay(300); 
      digitalWrite(ledPin, HIGH);
      delay(300); }
  
}

void WriteSig(){
  char intString[16];
  char fileName[30] = {};
  //strcpy(fileName, labelName);
  //char charNum[4];
  //itoa(fileNum, charNum, 10);
  strcpy(fileName, "");
  //strcat(fileName, charNum);
  itoa(label, intString, 10);
  strcat(fileName, intString);
  strcat(fileName, "_");
  itoa(repeats, intString, 10);
  strcat(fileName, intString);
  strcat(fileName, ".TXT");
//  Serial.print("FileName: ");
//  Serial.println(fileName);
  //delay(100);
  myFile = SD.open(fileName, FILE_WRITE);
  if(myFile){
    //Serial.println("Writing Data to SD...");
    
    myFile.print("{\"values\":[");
    for(int j = 0; j < signalIndex; j++){
      for(int i = 0; i < sigNum; i++){
        //Serial.println("Writing to test.txt...");
        myFile.print(signalVec[i][j]);
        if(!(j == (signalIndex - 1) && i == (sigNum - 1))) myFile.print(",");
      }
    }
    myFile.print("]");
//    myFile.print(",\"signal\":[");
//    myFile.print(sig);
//    myFile.print("]");
    myFile.print(",\"label\":[");
    myFile.print(label);
    myFile.print("]");
    myFile.print(",\"arrayDim\":[");
    myFile.print("[");
    myFile.print(sigNum);
    myFile.print("],");
    myFile.print("[");
    myFile.print(signalIndex);
    myFile.print("]");
    myFile.print("]}");
    myFile.println("");
    myFile.close();
  }else{
//    Serial.println("error opening :");
  }
  
}
