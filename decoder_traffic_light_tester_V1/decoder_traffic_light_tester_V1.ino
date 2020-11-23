/*
 * Decoder for traffic light with three or two leds on ARDUINO NANO : tester for the full config 
 * All colors will blink to allow the user to see which on is falty
 * 
 * 
 * TRAFFIC LIGHT WITH THREE leds (GREEN, RED, YELLOW)
 * 5 decoders of traffic lights with three leds on Arduino UNO per MCP23017
 * the leds are controlled by pins 0 to 14 on the MCP23017
 * led should be in the Green Red Yellow order as below
 * traffic light 1 : 0,1,2
 * traffic light 2 : 3,4,5
 * traffic light 3 : 6,7,8 (7 is pin 0 on bank B)
 * traffic light 4 : 9,10,11
 * traffic light 5 : 12,13,14 (pin 15 is not used)
 * 
 * 
 * CONFIGURATION
 * MODE : determined by the common of the led, LOW if common = HIGH, HIGH if common = LOW
 * NB_TRAFFIC_LIGHT : determined by the kind of traffic light, BICOLOR for two leds, TRICOLOR for three leds
 * 
 * V1 : first version starting from the main decoder program
 */

/***************************************************************************************           
 *            CONFIGURATION SETTING
 *            
 ****************************************************************************************/
 
#define CONSOLE                             // output console, delete or comment this after checking the configuration to avoid serial messages
#define MODE   HIGH                          // LOW or HIGH for CDM Standard : HIGH
                 
#define NB_TRAFFIC_LIGHT  TRICOLOR           //  TRICOLOR or BICOLOR 
 
// MCP support
#include <Wire.h>
#include "Adafruit_MCP23017.h" // The one that was working better with CQRobot shield

#define MCP23017_ADDR1 0x23  // I2C addr of the PCM23017 0x20 for the diymore 0x27 for the CQRobot, in case does not work  : use I2C_Scanner 
// #define MCP23017_ADDR2 0x23  // I2C addr for second CQRobot, A3 strapped on the board

Adafruit_MCP23017 mcp1; // with this one we can now address the 16 ports of the MCP using I2C
// Adafruit_MCP23017 mcp2; // with this one we can now address the 16 ports of the MCP using I2C

#define MCP1PORTNUMBER 16 //we will use only 0 to 14 as we have 3 pins per traffic light
#define MCP2PORTNUMBER 16


// traffic light

#define BICOLOR  8                     // 8 traffic lights with two leds
#define TRICOLOR 5                     // 5 traffic lights with three leds
#define FIRST_PIN         0             // pin of the first traffic light  on the MCP
#define GREEN             0             // address DCC/0
#define RED               1             // address DCC/1 
#define YELLOW            2             // address DCC+1/0

// traffic light definition

struct light {
  int address;                    // its DCC address
  int current_position;           // green / red / yellow
  int green;                      // pin of the green led
  int red;                        // pin of the red led
  int yellow;                     // pin of the yellow led
  boolean activation_request;     // request of activation
};
light traffic_light[NB_TRAFFIC_LIGHT];    // the set of traffic light

/********************************************************************
 * method called if a request is made by the DCC
 * 
 *******************************************************************/
void test_at_start() {  // cycle all lights color by color
    for (int j=0;j<3;j++) { 
      for (int i=0; i<5;i++)  mcp1.digitalWrite(i*3+j, LOW); //LED turns off
    }
  Serial.println("Lights off");
  
  for (int j=0;j<3;j++){
  for (int i=0; i<TRICOLOR;i++) {
  mcp1.digitalWrite(i*3+j, HIGH); //LED lights up
  Serial.print("On for ");Serial.println(i+3+j);  // for debug uncomment
  // mcp2.digitalWrite(i*3+j, HIGH); //LED lights up
  }
  delay(1000);
  for (int i=0; i<5;i++)  mcp1.digitalWrite(i*3+j, LOW); //LED turns off
  // for (int i=0; i<5;i++)  mcp2.digitalWrite(i*3+j, LOW); //LED turns off  
  }
  #ifdef CONSOLE
  Serial.println(" cycle finished");
  #endif            
  }

/**********************************************************************************************
 *  setup
 * 
 ******************************************************************************************/

void setup() {

 #ifdef CONSOLE
   Serial.begin(115200);
   Serial.println();
   Serial.println("Start... -------------");
   Serial.println(__FILE__);
   Serial.println(); 
   for (int i=0; i<NB_TRAFFIC_LIGHT; i++){
    Serial.print("traffic light");Serial.println(i);
    Serial.print("\t green led on pin : ");Serial.print(traffic_light[i].green);Serial.print(" , DCC address : ");Serial.print(traffic_light[i].address);Serial.println("/0"); 
    Serial.print("\t red led on pin : ");Serial.print(traffic_light[i].red);Serial.print(" , DCC address : ");Serial.print(traffic_light[i].address);Serial.println("/1");
    if (NB_TRAFFIC_LIGHT == TRICOLOR) {
      Serial.print("\t yellow led on pin : ");Serial.print(traffic_light[i].yellow);Serial.print(" , DCC address : ");Serial.print(traffic_light[i].address+1);Serial.println("/0");
      }
   }
  #endif


// MCP Setup overall on MCP 1
  byte mcp1error;
  Wire.begin(); 
  Wire.beginTransmission(MCP23017_ADDR1);
  mcp1error = Wire.endTransmission();  // check I2C 
  #ifdef CONSOLE
   if (mcp1error == 0) Serial.println("MCP1 connected OK"); else Serial.println("Something wrong on MCP1 connection");
  #endif
    /* start MCP on both banks */
  mcp1.begin(MCP23017_ADDR1,&Wire);

/*
// MCP Setup overall on MCP 2 DOES NOT WORK 
  byte mcp2error;
  Wire.begin(); 
  Wire.beginTransmission(MCP23017_ADDR2);
  mcp2error = Wire.endTransmission(MCP23017_ADDR2);  // check I2C 
  #ifdef CONSOLE
   if (mcp2error == 0) Serial.println("MCP2 connected OK"); else Serial.println("Something wrong on MCP2 connection");
  #endif
  */
    /* start MCP on both banks */
  // mcp2.begin(MCP23017_ADDR2);
  // MCP Setup open ports     
     for (int j=0;j<3;j++) { 
      for (int i=0; i<5;i++)  mcp1.pinMode(i*3+j, OUTPUT); //define pin for output
   /*  for (int j=0;j<3;j++) { 
      for (int i=0; i<5;i++)  mcp2.pinMode(i*3+j, OUTPUT); //define pin for outpu
    }*/
                                                            
  Serial.println("Let's go now ...");
}
}

/*************************************************************************
 * loop
 *************************************************************************/
 
void loop() {
  test_at_start();
  delay(100);
  }
  
