#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <LiquidCrystal.h>
#include <Math.h>
#include "RTClib.h"
//SD Card datalogger library and setup
#include <SD.h>

#define LOG_INTERVAL 1000
#define SYNC_INTERVAL 1000
#define ECHO_TO_SERIAL
#define redLEDpin 2
#define greenLEDpin 3
RTC_DS1307 RTC;

const int chipSelect = 10;

//from adafruit github
Adafruit_BMP085 bmp;

//lcd pins
//LiquidCrystal lcd(RS, EN, DB4, DB5, DB6, DB7);
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

//array of degree symbol for lcd
byte degrSym[8] = {
  B00110,
  B01001,
  B01001,
  B00110,
  B00000,
  B00000,
  B00000,
  B00000
};

//BMP085 declare variables
float tempC;
float tempF;
int presPA;
float presInHg;
int meters;
float feet;

//function to convert Celsius to Fahrenheit
float cToF(float c) {
  return ((c * 9.0) / 5.0) + 32.0;
}

//function to convert Pa to inches Hg
float paToInHg(float pa) {
  return pa * 0.000295333727;
}

//function to convert meters to feet
float mToFt(int m) {
  return m * 3.2808399;
}

void error(char *str) {
  Serial.print("error: ");
  Serial.println(str);
  
  digitalWrite(redLEDpin, HIGH);
  
  while(1);
}

void setup() {
  Serial.begin(9600);
//datalogger
  Serial.print("Initializing SD card...");
  pinMode(redLEDpin, OUTPUT);
  pinMode(greenLEDpin, OUTPUT);
  pinMode(10, OUTPUT);
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

//BMP085
  bmp.begin();

//LCD
  lcd.createChar(0, degrSym);
  lcd.begin(16, 2);
}

void loop() {
    DateTime now;
    delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));
    
    digitalWrite(greenLEDpin, HIGH);
    
    now = RTC.now();
    
  //initialize BMP085 variables
    float tempC = 0;
    int presPa = 0;
    int meters = 0;
    float tempf = 0.0;

  //initialize logger variables
    String dataString = "";

  //read sensor values into variables
    tempC = bmp.readTemperature();
    tempF = floor(cToF(tempC) * 10 + 0.5) / 10;
    presPa = bmp.readPressure();
    presInHg = paToInHg(presPa);
    meters = bmp.readAltitude(101760);
    feet = mToFt(meters);

  //ouput to serial
    Serial.print("Temperature = ");
    Serial.print(tempC);
    Serial.println(" *C");
    Serial.print(tempF);
    Serial.println(" *F");

    Serial.print("Pressure = ");
    Serial.print(presPa);
    Serial.println(" Pa");

    Serial.println();

  //output to lcd screen
    lcd.setCursor(0,0);
    lcd.print(tempF,1);
    lcd.write(byte(0));
    lcd.print("F");

    lcd.setCursor(8,0);
    lcd.print(paToInHg(bmp.readPressure()));
    lcd.print(" Hg");

    //lcd.rightToLeft();
    lcd.setCursor(7,1);
    if (feet <= 9)
      lcd.print(" ");
    if (feet <= 99)
      lcd.print(" ");
    lcd.print(feet);
    lcd.print(" ft");

// log to sd card
  File dataFile = SD.open("LOG.CSV", FILE_WRITE)
  
  ;
 
  Serial.print("Logging to: ");
  Serial.println(dataFile);
  
  if (dataFile) {
    dataFile.print(now.unixtime());
    dataFile.print(",");
    dataFile.print(now.year(), DEC);
    dataFile.print("/");
    dataFile.print(now.month(), DEC);
    dataFile.print("/");
    dataFile.print(now.day(), DEC);
    dataFile.print(" ");
    dataFile.print(now.hour(), DEC);
    dataFile.print(":");
    dataFile.print(now.minute(), DEC);
    dataFile.print(":");
    dataFile.print(now.second(), DEC);
    dataFile.print(",");
    dataFile.print(tempF);
    dataFile.print(",");
    dataFile.print(bmp.readPressure());
    dataFile.print(",");
    dataFile.println(feet);
    dataFile.close();
    
    digitalWrite(greenLEDpin, LOW);
    
    
    
    Serial.println(tempF);
  }
  else {
    Serial.println("error opening datalog.txt");
  }
  
    delay(500);
}



// printFloat prints out the float 'value' rounded to 'places' places after the decimal point
float printFloat(float value, int places) {
  // this is used to cast digits
  int digit;
  float tens = 0.1;
  int tenscount = 0;
  int i;
  float tempfloat = value;

    // make sure we round properly. this could use pow from <math.h>, but doesn't seem worth the import
  // if this rounding step isn't here, the value  54.321 prints as 54.3209

  // calculate rounding term d:   0.5/pow(10,places)
  float d = 0.5;
  if (value < 0)
    d *= -1.0;
  // divide by ten for each decimal place
  for (i = 0; i < places; i++)
    d/= 10.0;
  // this small addition, combined with truncation will round our values properly
  tempfloat +=  d;

  // first get value tens to be the large power of ten less than value
  // tenscount isn't necessary but it would be useful if you wanted to know after this how many chars the number will take

  if (value < 0)
    tempfloat *= -1.0;
  while ((tens * 10.0) <= tempfloat) {
    tens *= 10.0;
    tenscount += 1;
  }


  // write out the negative if needed
  if (value < 0)
    Serial.print('-');

  if (tenscount == 0)
    Serial.print(0, DEC);

  for (i=0; i< tenscount; i++) {
    digit = (int) (tempfloat/tens);
    Serial.print(digit, DEC);
    tempfloat = tempfloat - ((float)digit * tens);
    tens /= 10.0;
  }

  // if no places after decimal, stop now and return
  if (places <= 0)
    return value;

  // otherwise, write the point and continue on
  Serial.print('.');

  // now write out each decimal place by shifting digits one by one into the ones place and writing the truncated value
  for (i = 0; i < places; i++) {
    tempfloat *= 10.0;
    digit = (int) tempfloat;
    Serial.print(digit,DEC);
    // once written, subtract off that digit
    tempfloat = tempfloat - (float) digit;
  }
}
