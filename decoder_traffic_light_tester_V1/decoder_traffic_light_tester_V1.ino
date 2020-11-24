/*
 * Decoder for traffic light with three or two leds on ARDUINO UNO/NANO : tester for the full config 
 * All colors will blink to allow the user to see which on is falty
 * 
 * V1_1
 * - create array of MCP to allow maximum of MCP per arduino
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
 * MCP use with adafruit lib
 * 
 * MCP number in begin, statuses od A0 A1 A2, I2C address
 * addr 0 = A2 low , A1 low , A0 low  000, 0x20
 * addr 1 = A2 low , A1 low , A0 high 001 , 0x21
 * addr 2 = A2 low , A1 high , A0 low  010, 0x22
 * addr 3 = A2 low , A1 high , A0 high  011, 0x23
 * addr 4 = A2 high , A1 low , A0 low  100, 0x24
 * addr 5 = A2 high , A1 low , A0 high  101, 0x25
 * addr 6 = A2 high , A1 high , A0 low  110, 0x26
 * addr 7 = A2 high, A1 high, A0 high 111, 0x27
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
 
// MCP support
#include <Wire.h>
#include "Adafruit_MCP23017.h" // The one that was working better with CQRobot shield

#define INSTALLEDMCPNB 3 // number of installed MCP
#define DELAY1 3000
#define DELAY2 2000

// having this structure will allow to match the phisical setup of your mcps
struct mcpchip {
  int mcpnum; // to be used with adafruit MCP23017 library to initiate (begin)
  int mcpi2caddr; // I2C address, has to match the mcpnum num above 
  int usedport; // really used ports has to be lower than 16 
};

mcpchip mcp_board[8] = {
  {7,0x27,16},
  {3,0x23,16},
  {2,0x22,16},
  {1,0x21,16},
  {4,0x24,16},
  {5,0x25,16},
  {0,0x20,16}
}; 

Adafruit_MCP23017 mcps[INSTALLEDMCPNB];

void test_at_start() {  // cycle all lights color by color
  for (int i=0;i<INSTALLEDMCPNB;i++) {
    for (int j=0;j<mcp_board[i].usedport;j++) {
      mcps[i].digitalWrite(j, LOW); //LED turns off
      }
    }
    Serial.println("Lights off");
    
    for (int k=0; k<3;k++) { //go through the 3 colors
      Serial.println();Serial.print("color cycle started for color ");Serial.println(k); 
      for (int i=0;i<INSTALLEDMCPNB;i++){ //go through the various boards
        for (int j=0; j<int(mcp_board[i].usedport/3);j++) { // go trough the tricolor traffic lights 
          mcps[i].digitalWrite(j*3+k, HIGH); //LED lights up
          Serial.print(" On for ");Serial.print(j*3+k);  // for debug uncomment
        }
       }
        delay(DELAY1);
    
          for (int i=0;i<INSTALLEDMCPNB;i++) { // lights off
              for (int j=0;j<mcp_board[i].usedport;j++) {
              mcps[i].digitalWrite(j, LOW); //LED turns off
               }
          }
      delay(DELAY2);
      Serial.println();Serial.print("color cycle finished for color ");Serial.println(k);      
    }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
     Serial.println();
   Serial.println("Start... -------------");
   Serial.println(__FILE__);
   Serial.println(); 

  for (int i=0;i<INSTALLEDMCPNB;i++) {
    byte mcperror;
    // wire method is not required with Adafruit lib, but this alloows to test the MCP at I2C level
    Wire.begin(); 
    Wire.beginTransmission(mcp_board[i].mcpi2caddr);
    mcperror = Wire.endTransmission();  // check I2C 
    if (mcperror == 0) { Serial.print("MCP");Serial.print(i);Serial.print(" connected OK at "); Serial.println(mcp_board[i].mcpi2caddr);}
      else { Serial.print("MCP");Serial.print(i);Serial.print(" NOT connected OK at "); Serial.println(mcp_board[i].mcpi2caddr);}
    Serial.print("Begin on board ");
    Serial.print(mcp_board[i].mcpnum);
    mcps[i].begin(mcp_board[i].mcpnum);
    // now declare ports for output
    for (int j=0;j<mcp_board[i].usedport;j++) {
      mcps[i].pinMode(j, OUTPUT);
      Serial.print(" output MCP");Serial.print(i);Serial.print(" port ");Serial.print(j);
    }
  }
  Serial.println("et c'est parti..");
  

}

void loop() {
  // put your main code here, to run repeatedly:
  test_at_start();
}
