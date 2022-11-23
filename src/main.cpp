#include <Wire.h>
#include <SFE_BMP180.h>
#include <SPIMemory.h>
#include <Arduino.h>

#define BAUD_RATE 9600
#define CS_PIN 7
#define RXLED 17

struct flight_counts{
 int num_flights;
 int flight_addr_strt[10];
 int flight_length[10];
} flights;

SPIFlash flash(CS_PIN);
SFE_BMP180 bmp180;

// Program Related variables
float a,P; // altitude and pressure
float t; // time
double baseline; //baseline pressure used for calculations

long time = 0;
int poll = 1000 / 10;
int current_address = -1;
long flights_adr = 256 * 35000; //page size * page number


double getPressure() {
  char status;
  double T,P,p0,a;
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
  flash.begin();
  
  /** Testing for Serial Connection **/
  bool is_serial = false;
  char do_read = 'q';
  char do_erase = 'q';
  for(int i = 0; i < 5; i++) {
    digitalWrite(RXLED, HIGH);
    delay(500);
    digitalWrite(RXLED, LOW);
    if(Serial) {
      is_serial = true;
      i = 6;
    }
  }

  /**making sure we get and ID off of the chip **/
  int id = flash.getJEDECID();
  //Serial.println(id);
  if (id) {
    digitalWrite(RXLED, LOW);
    delay(50);
    digitalWrite(RXLED, HIGH);
    delay(50);
    digitalWrite(RXLED, LOW);
    delay(50);
    digitalWrite(RXLED, HIGH);
  } else {
    digitalWrite(RXLED, LOW);
    Serial.println("No comms. Check wiring. Is chip supported? If unable to fix, raise an issue on Github");
  }

  time = millis();

  bool can_read = flash.readAnything(flights_adr,flights);
  Serial.print(flights.num_flights);
  Serial.print(" flights recorded\n");

  if(is_serial) {
    Serial.println("Would you like to read data y/n");
    while(do_read != 'y' && do_read != 'n') {
      delay(1000);
      do_read = Serial.read();
    }
    Serial.println(do_read);
  }
  bool reading = false;
  //Serial.println(can_read);
  if(do_read == 'y') {
    reading = true;
   
    for(int i = 0; i < flights.num_flights; i++) {
      
      int fl_strt = flights.flight_addr_strt[i];
      int lgth = flights.flight_length[i];
      Serial.print("reading flight starting at: ");
      Serial.println(fl_strt);
      Serial.print("reading flight with length: ");
      Serial.println(lgth);
      byte data[lgth];
      if(lgth > 0 && fl_strt >= 0 && flash.readByteArray(fl_strt,data,lgth)) {
        float * float_data = (float *)(data);
        Serial.print(" altitude, ");
        Serial.print(" Pressure, ");
        Serial.println(" Time, ");
        for(int j = 0; j < lgth / 8; j+=3) {
          Serial.print(float_data[j]);
          Serial.print("\t");
          Serial.print(float_data[j+1]);
          Serial.print("\t");
          Serial.println(float_data[j+2]);
        }
      } else {
        Serial.println("Unable to read from chip!");
      }
    }
    reading = false;
  
  } else {
    flights.num_flights++; //update to a new flight
    flights.flight_length[flights.num_flights-1] = 0; //setting the length from -1 to 0
  }
  while(reading);
  if(is_serial) {
    Serial.println("Would you like to erase chip y/n");
    while(do_erase != 'y' && do_erase != 'n') {
        delay(1000);
        do_erase = Serial.read();
    }
    Serial.println(do_erase);
  }

  if(do_erase == 'y') {
    Serial.println("erasing will begin in 2 seconds...");
    delay(2000);
    Serial.println("erasing...");
    if(flash.eraseChip()) {
    Serial.print("erase successful in ");
    long e_time = millis() - time;
    Serial.print(e_time / 60000);
    Serial.print("min and ");
    Serial.print((float) (e_time % 60000) / 1000.0);
    Serial.println("sec");
    flights.num_flights = 0;
    flash.eraseSector(flights_adr);
    flash.writeAnything(flights_adr, flights);
    } else {
      Serial.println("could not erase!");
      while(1);
    }
  }

  if(Serial) {
    while(1);
  }

  if (bmp180.begin()) {
    Serial.println("BMP180 init success");
  } else {
    Serial.println("bmp unsuccessful");
    digitalWrite(RXLED, LOW);
    while (1);
  }
  
  baseline = getPressure();
  flash.eraseSector(flights_adr);
  flash.writeAnything(flights_adr,flights);
}



void loop() {
   // find the next empty spot
  digitalWrite(RXLED, HIGH);
  if((int) millis() - (int) t >= poll) {
    digitalWrite(RXLED, LOW);
    P = (float) getPressure();
    a = (float) bmp180.altitude(P,baseline);
    t = (float) millis();
    
    int free_add = flash.getAddress(sizeof(a));
    if(current_address < 0)
      flights.flight_addr_strt[flights.num_flights - 1] = free_add;
    if (flash.writeByteArray(free_add, ((byte *)&a), sizeof(a))) {
      Serial.print(a);
      Serial.print(" m, ");
    } else {
      digitalWrite(RXLED, LOW);
      Serial.println("write failed :(");
    }
    
    free_add = flash.getAddress(sizeof(P));
    if (flash.writeByteArray(free_add, ((byte *)&P), sizeof(P))) {
      Serial.print(P);
      Serial.print(" atm, ");
    } else {
      Serial.println("write failed :(");
      digitalWrite(RXLED, LOW);
    }

    free_add = flash.getAddress(sizeof(t));
    if (flash.writeByteArray(free_add, ((byte *)&t), sizeof(t))) {
      Serial.print(t);
      Serial.println(" ms, ");
    } else {
      digitalWrite(RXLED, LOW);
      Serial.println("write failed :(");
    }
    current_address = free_add;
    Serial.println(current_address);
    flights.flight_length[flights.num_flights - 1] += (4 * 3);
    Serial.println("writing flight length...");
    flash.eraseSector(flights_adr);
    flash.writeAnything(flights_adr, flights);
  }
}