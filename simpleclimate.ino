
#include<stdlib.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include<i2c_base.h>
#include <si7021_u.h>

#include "U8glib.h"
#include "APDS9930.h"

#include"atimer.h"

#define APDS9930_INT    2  // Needs to be an interrupt pin

// Constants
#define PROX_INT_HIGH   600 // Proximity level for interrupt
#define PROX_INT_LOW    0  // No far interrupt

#define OLED_MOSI   9
#define OLED_CLK   10
#define OLED_DC    11
#define OLED_CS    12
#define OLED_RESET 13

APDS9930 apds = APDS9930();

Si7021_Unified si7021 = Si7021_Unified();
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);

U8GLIB_SSD1306_ADAFRUIT_128X64 u8g(OLED_CLK, OLED_MOSI, OLED_CS, OLED_DC);

float temp,pres,hum;
sensors_event_t event;
float seaLevelPressure = SENSORS_PRESSURE_SEALEVELHPA;
char charVal[10]={0};
uint16_t proximity_data = 0;
volatile bool isr_flag = false;


void draw(void) {
  // graphic commands to redraw the complete screen should be placed here  
  u8g.setFont(u8g_font_unifont);
  //u8g.setFont(u8g_font_osb21);
  //u8g.setFont(u8g_font_9x15);
  u8g.drawStr( 0, 10, "Temp,Hum & pressure");
  
  dtostrf(temp, 4, 1, charVal);
  u8g.drawStr( 0, 30, charVal);

  dtostrf(hum, 4, 2, charVal);
  u8g.drawStr( 0, 45, charVal);

  dtostrf(pres, 4, 2, charVal);
  u8g.drawStr( 0, 60, charVal);
}

void show(void){
    u8g.firstPage();  
  do {
    draw();
  } while( u8g.nextPage() );

}

void sendValues(void){
    Serial.print("D|");
    
    Serial.print(hum);
    Serial.print("|");
    
    Serial.print(temp);
    Serial.print("|");

    pres = event.pressure;
    Serial.print(pres);
    
    Serial.println("");
}

void getValues(void){
      si7021.getEvent(&event);
    hum = event.relative_humidity;

    si7021.getTemperature(temp);

    bmp.getEvent(&event);
    pres = event.pressure;

    bmp.getTemperature(&temp);

}

void displayOff(){
     u8g.sleepOn();
}

void sendAndShow(){
  getValues();    
  show();
  sendValues();
}
  
ATimer displayTimer(&displayOff);
ATimer sendTimer(&sendAndShow,false);
#define COUNT 2
ATimer *timers[COUNT]={&displayTimer,&sendTimer};


void setup() {
    Serial.begin(9600);
    Serial.println("Pressure Sensor Test"); Serial.println("");

    /* Initialise the humidity sensor */
    if(!si7021.begin())
    {
       /* Problem to find sensor*/
       Serial.print("Can not find s17021 sensor!");
       while(1);
    }
    /* Initialise the sensor */
   if(!bmp.begin())
   {
     /* There was a problem detecting the BMP085 ... check your connections */
     Serial.print("Ooops, no BMP085 detected ... Check your wiring or I2C ADDR!");
     while(1);
   }

   // Initialize APDS-9930 (configure I2C and initial values)
  pinMode(APDS9930_INT, INPUT);
  // Initialize interrupt service routine
  attachInterrupt(0, interruptRoutine, FALLING);
  
  if ( apds.init() ) {
    Serial.println(F("APDS-9930 initialization complete"));
  } else {
    Serial.println(F("Something went wrong during APDS-9930 init!"));
  }
  // Adjust the Proximity sensor gain
  if ( !apds.setProximityGain(PGAIN_2X) ) {
    Serial.println(F("Something went wrong trying to set PGAIN"));
  }
  
  // Set proximity interrupt thresholds
  if ( !apds.setProximityIntLowThreshold(PROX_INT_LOW) ) {
    Serial.println(F("Error writing low threshold"));
  }
  if ( !apds.setProximityIntHighThreshold(PROX_INT_HIGH) ) {
    Serial.println(F("Error writing high threshold"));
  }
    
  // Start running the APDS-9930 proximity sensor ( interrupts)
  if ( !apds.enableProximitySensor(true) ) {
    Serial.println(F("Something went wrong during sensor init!"));
  }

  // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255,255,255);
  }
  pinMode(OLED_RESET, OUTPUT);
  digitalWrite(OLED_RESET, HIGH);
  
  sendTimer.start(5000);
  displayTimer.start(10000);
  sendAndShow();
}

void loop() {
    //apds.readProximity(proximity_data);
    if ( isr_flag ){
      u8g.sleepOff();
      displayTimer.start(5000);
      // Reset flag and clear APDS-9930 interrupt (IMPORTANT!)
      isr_flag = false;
      if ( !apds.clearProximityInt() ) {
        Serial.println("Error clearing interrupt");
      }
    }
 

     for(int i=0;i < COUNT;i++ )
         timers[i]->run();
    delay(10);
}

void interruptRoutine() {
  isr_flag = true;
}
