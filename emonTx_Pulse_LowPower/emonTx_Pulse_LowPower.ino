/*
  EmonTx Pulse Low Power example

  Many meters have pulse outputs, including electricity meters: single phase, 3-phase, 
  import, export.. Gas meters, Water flow meters etc

  The pulse output may be a flashing LED or a switching relay (usually solid state) or both.

  In the case of an electricity meter a pulse output corresponds to a certain amount of 
  energy passing through the meter (Kwhr/Wh). For single-phase domestic electricity meters
  (eg. Elster A100c) each pulse usually corresponds to 1 Wh (1000 pulses per kwh).  

  The code below detects the falling edge of each pulse and increment pulseCount
  
  It calculated the power by the calculating the time elapsed between pulses.
  
  Read more about pulse counting here:
  http://openenergymonitor.org/emon/buildingblocks/introduction-to-pulse-counting
 
  -----------------------------------------
  Part of the openenergymonitor.org project
  Licence: GNU GPL V3
 
  Authors: Glyn Hudson, Trystan Lea, Peter Clarke.
  Builds upon JeeLabs RF12 library and Arduino

  THIS SKETCH REQUIRES:

  Libraries in the standard arduino libraries folder:
 	- JeeLib		https://github.com/jcw/jeelib

  Other files in project directory (should appear in the arduino tabs above)
	- emontx_lib.ino
*/

#define freq RF12_868MHZ                                                // Frequency of RF12B module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.433MHZ, RF12_868MHZ or RF12_915MHZ. You should use the one matching the module you have.
const int nodeID = 10;                                                  // emonTx RFM12B node ID
const int networkGroup = 210;                                           // emonTx RFM12B wireless network group - needs to be same as emonBase and emonGLCD needs to be same as emonBase and emonGLCD

const int UNO = 1;                                                      // Set to 0 if your not using the UNO bootloader (i.e using Duemilanove) - All Atmega's shipped from OpenEnergyMonitor come with Arduino Uno bootloader
#include <avr/wdt.h>// the UNO bootloader 

#include <JeeLib.h>                                                     // Download JeeLib: http://github.com/jcw/jeelib
ISR(WDT_vect) { Sleepy::watchdogEvent(); }                              // Attached JeeLib sleep function to Atmega328 watchdog -enables MCU to be put into sleep mode inbetween readings to reduce power consumption
  
typedef struct { int power, pulse, batt;} PayloadTX;
PayloadTX emontx;                                                       // neat way of packaging data for RF comms

const int LEDpin = 9;  

const long time_between_readings = 900000;                              // Spend some time in low-power mode, the timing is only approximate.

// Pulse counting settings 
long pulseCount = 0;                                                    // Number of pulses, used to measure energy.
unsigned long pulseTime,lastTime;                                       // Used to measure power.
double power, elapsedWh;                                                // power and energy
int ppwh = 1;                                                           // 1000 pulses/kwh = 1 pulse per wh - Number of pulses per wh - found or set on the meter.


void setup() 
{
  Serial.begin(9600);
  Serial.println("emonTX Pulse example");
  delay(100);
             
  rf12_initialize(nodeID, freq, networkGroup);                          // initialize RF
  rf12_sleep(RF12_SLEEP);

  //pinMode(LEDpin, OUTPUT);                                              // Setup indicator LED
  //digitalWrite(LEDpin, HIGH);
  
  attachInterrupt(1, onPulse, FALLING);                                 // KWH interrupt attached to IRQ 1  = pin3 - hardwired to emonTx pulse jackplug. For connections see: http://openenergymonitor.org/emon/node/208
}


unsigned long last_rf;

//Main Program Loop.
void loop() 
{ 
 unsigned long time_since_last_rf = millis()-last_rf;
 Serial.print("Time since last TX "); Serial.println(time_since_last_rf);
 delay(50);
    if (time_since_last_rf >= time_between_readings) {                                       //If it is equal or greater than time between readings since the last tx then send the current values over RF.
      emontx.pulse = pulseCount; pulseCount=0;
      emontx.batt = readVcc(); 
      send_rf_data();  // *SEND RF DATA* - see emontx_lib
      last_rf = millis();
      time_since_last_rf = 0;
      Serial.print(emontx.power);
      Serial.print("W ");
      Serial.println(emontx.pulse);
      //digitalWrite(LEDpin, HIGH); delay(2); digitalWrite(LEDpin, LOW);      // flash LED
    }
 unsigned long backtosleep = (time_between_readings-time_since_last_rf);      //Calculate the time the MCU needs to go back to sleep for.
 Serial.print("Going to sleep for "); Serial.println(backtosleep);
 delay(50);
 Sleepy::loseSomeTime(backtosleep);    //Put the MCU into low power mode.
}

// The interrupt routine - runs each time a falling edge of a pulse is detected
void onPulse()                  
{
  lastTime = pulseTime;        //used to measure time between pulses.
  pulseTime = micros();
  pulseCount++;                                                      //pulseCounter               
  emontx.power = int((3600000000.0 / (pulseTime - lastTime))/ppwh);  //Calculate power
}

// Calculate MCU Voltage (mV).
long readVcc() {
  long result;
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2);
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result;
  return result;
}
