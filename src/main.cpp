#include <Wire.h>
#include <SFE_BMP180.h>
#include <SPIMemory.h>
#include <Arduino.h>
#define Serial SERIAL_PORT_USBVIRTUAL

#define BAUD_RATE 9600
#define CS_PIN 7


SPIFlash flash(CS_PIN);
//bool readSerialStr(String &inputStr);


SFE_BMP180 bmp180;
double baseline;
int current_address;
int RXLED = 17; 

// Byte Arrays
byte * BtAr(double num1, double num2, long num3) {
  byte arr[24];
  for (int i = 0; i < 3; i++) {
    Serial.println(i);
    switch (i) {
      case 0:
        for (int j = 0; j < 8; j++)
          arr[j] = ((byte *)&num1)[j];
        Serial.println("phew");
        break;
      case 1:
        for (int j = 0; j < 8; j++)
          arr[j+8] = ((byte *)&num2)[j];
        break;
      case 2:
        for (int j = 0; j < 8; j++)
          arr[j+16] = ((byte *)&num3)[j];
        break;
      default:
        Serial.println("Ya done fuqed up");
    }
  }
}

double getPressure() {
  char status;
  double T,P,p0,a;
  Serial.println("in getPressure");

  status = bmp180.startTemperature();
  if (status != 0) {
    delay(status);
    status = bmp180.getTemperature(T);
    if (status != 0) {
      status = bmp180.startPressure(3);
      if (status != 0) {
        delay(status);
        status = bmp180.getPressure(P,T);
        if (status != 0) {
          Serial.println("got pressure!");
          return(P);
        }
        else Serial.println("error retrieving pressure measurement\n");
        digitalWrite(RXLED, LOW);
      }
      else Serial.println("error starting pressure measurement\n");
      digitalWrite(RXLED, LOW);
    }
    else Serial.println("error retrieving temperature measurement\n");
    digitalWrite(RXLED, LOW);
  }
  else Serial.println("error starting temperature measurement\n");
  digitalWrite(RXLED, LOW);
}


// Real Code Time
void setup() {
  Serial.begin(BAUD_RATE);
  pinMode(RXLED, OUTPUT);
  
  while(!Serial);

  if (bmp180.begin()) {
    Serial.println("BMP180 init success");
    digitalWrite(RXLED, HIGH);

  } else {
    //while (1);
    Serial.println("bmp unsuccessful");
  }
    delay(200);
  baseline = getPressure();
}

void loop() {
  Serial.println("in the loop!");
  delay(500);
  double a,P;
  long t;
  P = getPressure();
  Serial.println("got P");
  a = bmp180.altitude(P,baseline);
  Serial.println("got a");
  t = millis();
  Serial.println("got t");
  // call function to get array of bytes
  byte * arrStore = BtAr(a, P, t);
  Serial.println("russell's BS worked maybe");
  // find the next empty spot
  
  if (flash.writeByteArray(current_address, arrStore, 24)) {
    current_address += 24;
    digitalWrite(RXLED, LOW);
  } else {
    Serial.println("write failed :(");
  }

  if(Serial){
    Serial.println("thank you for plugging me in daddy");
  }

}