/*
 * Decoder for traffic light with three or two leds on ARDUINO NANO
 * 
 * MAny thanks to (i hope i do not forget anybody)
 * - Minabay library https://github.com/MynaBay/DCC_Decoder
 * - Informations and main code https://www.locoduino.org/spip.php?article161 
 * - i used a simple connection to DCC (see
 * 
 * pin 2 receives DCC interrupts
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
 * two addresses by traffic light due to CDM Rail software first address has to be odd (see FIRST_ID_DCC)
 * ODD addresses for green and red lights
 * EVEN addresses for yellow light
 * 
 * (2 leds not implemented in this version)
 * TRAFFIC LIGHT WITH TWO leds (GREEN, RED) 
 * 8 decoders of traffic lights with two leds on Arduino UNO
 * the leds are controlled by pins 3 to A4 by pair
 * traffic light 1 : 3,4
 * traffic light 2 : 5,6
 * traffic light 3 : 7,8
 * traffic light 4 : 9,10
 * traffic light 5 : 11,12
 * traffic light 6 : 13,A0
 * traffic light 7 : A1,A2
 * traffic light 8 : A3,A4
 * one address by traffic light
 * 
 * CONFIGURATION
 * MODE : determined by the common of the led, LOW if common = HIGH, HIGH if common = LOW
 * FIRST_ID_DCC : DCC address of the first traffic light
 * NB_TRAFFIC_LIGHT : determined by the kind of traffic light, BICOLOR for two leds, TRICOLOR for three leds
 * 
 * V1 : minimal version works with CDM Rail and Signal 1 directly on the Arduino board
 * On CDM Rail
 * Signal timing has to be set to LDT_LS_DEC
 * Works OK
 * 
 * V1_2
 * Introduce MCP23017 to allow more signals in the future
 * works with I2C with SDA/A4 SCL/A5 on nano
 * 
 * Backlog
 * - test ESP32 Heltec
 * - add multiple MCP23017
 * - test low cost MCP23017
 * 
 */

/***************************************************************************************           
 *            CONFIGURATION SETTING
 *            
 ****************************************************************************************/
 
#define CONSOLE                             // output console, delete or comment this after checking the configuration to avoid serial messages
#define MODE   HIGH                          // LOW or HIGH for CDM Standard : HIGH
#define FIRST_ID_DCC   203                   // first DCC address, DCC_CODE
#define NB_TRAFFIC_LIGHT  TRICOLOR           //  TRICOLOR or BICOLOR 

/**********************************************************************************  
 *   DON'T CHANGE THE FOLLOWING
 *********************************************************************************/
 
/******************************************************************************
 *    INTERNAL PARAMETERS
 *    
 ********************************************************************************/
 
// MCP support
#include <Wire.h>
#include "Adafruit_MCP23017.h" // The one that was working better with CQRobot shield

#define MCP23017_ADDR 0x27  // I2C addr of the PCM23017 0x20 for the diymore 0x27 for the CQRobot, in case does not work  : use I2C_Scanner 
Adafruit_MCP23017 mcp1; // with this one we can now address the 16 ports of the MCP using I2C
#define MCP1PORTNUMBER 16


//  DCC

#include <DCC_Decoder.h>                 // Minabay library
#define kDCC_INTERRUPT    0             // pin 2 receives DCC interrupts
int previous_address = 0;               // avoids multiple DCC addresses
int previous_position = 2;              // avoids multiple DCC orders
volatile boolean update_light;          // set if an update should be processed after DCC control

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
  for (int j=0;j<3;j++){
  for (int i=0; i<TRICOLOR;i++) {
  mcp1.digitalWrite(i*3+j, HIGH); //LED lights up
  }
  delay(1000);
  for (int i=0; i<5;i++)  mcp1.digitalWrite(i*3+j, LOW); //LED turns off
  }
  #ifdef CONSOLE
  Serial.println(" intial test finished");
  #endif            
  }

void activation_traffic_light() {
  for (int i = 0; i < NB_TRAFFIC_LIGHT; i++)                 // for all traffic lights
  {
    if (traffic_light[i].activation_request == true)  // if the traffic_light is waiting for activation
    {
      #ifdef CONSOLE
       if (i == 0) {
         Serial.print("ON : Green : "); Serial.print(traffic_light[i].green);
         Serial.print(" red : ");Serial.print(traffic_light[i].red);
         Serial.print(" Yel : ");Serial.print(traffic_light[i].yellow);
         Serial.print(" mode : ");Serial.print(MODE);Serial.println();
         
        }
      #endif
      switch(traffic_light[i].current_position)            // we look the current position
      { 
        case GREEN :{                                                     // if green led
                      mcp1.digitalWrite(traffic_light[i].green,MODE);         // switch on green
                      mcp1.digitalWrite(traffic_light[i].red,!MODE);          // switch off red
                      if ( NB_TRAFFIC_LIGHT == TRICOLOR){mcp1.digitalWrite(traffic_light[i].yellow,!MODE);} // switch off yellow
                      #ifdef CONSOLE
                        Serial.print("activation -> traffic light");Serial.print(i+1);Serial.println(" : Green led");
                      #endif
                      break;}
        case RED : {                                                  // if red led
                      mcp1.digitalWrite(traffic_light[i].green,!MODE);      // switch off green
                      mcp1.digitalWrite(traffic_light[i].red,MODE);         // switch on red
                      if ( NB_TRAFFIC_LIGHT == TRICOLOR){mcp1.digitalWrite(traffic_light[i].yellow,!MODE);} // switch off yellow
                      #ifdef CONSOLE
                        Serial.print("activation -> traffic light");Serial.print(i+1);Serial.println(" : Red led");
                      #endif
                     break;}
         case YELLOW : {                                                  // if yellow led
                      mcp1.digitalWrite(traffic_light[i].green,!MODE);         // switch off green 
                      mcp1.digitalWrite(traffic_light[i].red,!MODE);           // switch off red
                      mcp1.digitalWrite(traffic_light[i].yellow,MODE);         // switch on yellow
                      #ifdef CONSOLE
                        Serial.print("activation -> traffic light");Serial.print(i+1);Serial.println(" : Yellow led");
                      #endif                      
                     break;}
         }
           }
       traffic_light[i].activation_request = false;            // the traffic light is updated
      }
  update_light = false;                                        // all updates are made
}

/************************************************************************************* 
 *  DCC method
 *  
 ***********************************************************************************/

void BasicAccDecoderPacket_Handler(int address, boolean activate, byte data)
{  
  
  address -= 1; address *= 4; address += 1; address += (data & 0x06) >> 1;    // DCC address decoding
  int led = (data & 0x01) ? GREEN : RED;                                      // DCC/0 or DCC/1
  int traffic_light_index = address;                                          // index of a traffic light   
  int color = led;                                                            // the color of the led
  boolean activation = false;
  if ((address != previous_address) || ((led != previous_position) && (address == previous_address))){ // if we change the address or the led
        switch (NB_TRAFFIC_LIGHT) {
          case BICOLOR : {      // if the address is in our range for traffic light with two leds
                    if ((address >= FIRST_ID_DCC) && (address < FIRST_ID_DCC + NB_TRAFFIC_LIGHT)){ 
                      traffic_light_index = address - FIRST_ID_DCC;                                  // index of the traffic light
                      activation = true;}
                        break;}
          case TRICOLOR : {     // if the address is in our range for traffic light with three leds
                    if ((address >= FIRST_ID_DCC) && (address < FIRST_ID_DCC + (2*NB_TRAFFIC_LIGHT))){ Serial.print("Addr : ");Serial.println(address);
                      if (address%2 == 0) {  traffic_light_index = address - 1; color = YELLOW;}         // if even address => yellow led
                        traffic_light_index = (traffic_light_index - FIRST_ID_DCC)/2;                    // index of the traffic light
                        activation = true;}
                        break;}
        }
     traffic_light[traffic_light_index].activation_request =  activation;    // activation is requested
     traffic_light[traffic_light_index].current_position = color;            // state is requested (color of the led)
     update_light = activation;                                              // traffic light update is requested
    }
   previous_address = address; previous_position = led;                    // the current activation is saved
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


// MCP Setup overall
  byte mcp1error;
  Wire.begin(); 
  Wire.beginTransmission(MCP23017_ADDR);
  mcp1error = Wire.endTransmission();  // check I2C 
  #ifdef CONSOLE
   if (mcp1error == 0) Serial.println("MCP1 connected OK"); else Serial.println("Something wrong on MCP1 connection");
  #endif
    /* start MCP on both banks */
  mcp1.begin(MCP23017_ADDR);
  // MCP Setup open ports 
  
  int pin_jump = 0;                                                     // a jump for traffic light pins
  int traffic_light_jump = 0;                                           // a jump for traffic light number
  for (int i=0; i<NB_TRAFFIC_LIGHT; i++){                               // for all the traffic lights
    traffic_light[i].activation_request = false;                        // no activation request
    traffic_light[i].green = pin_jump + FIRST_PIN;                      // pin number of the green led
    mcp1.pinMode(traffic_light[i].green, OUTPUT);                            // green led in output(ID DCC/0)                                                                                                                                                                                                                                                                   
    mcp1.digitalWrite(traffic_light[i].green, !MODE);                       // green led switch off
    traffic_light[i].red = 1+ pin_jump + FIRST_PIN;                    // pin number of the red led
    mcp1.pinMode(traffic_light[i].red, OUTPUT);                             // red led in output  (ID DCC/1)
    mcp1.digitalWrite(traffic_light[i].red, MODE);                          // red led switch on
    if (NB_TRAFFIC_LIGHT == TRICOLOR) {                                // if three leds
      traffic_light[i].address = traffic_light_jump + FIRST_ID_DCC + i;   // its DCC ID
      traffic_light[i].yellow = 2+ pin_jump + FIRST_PIN;               // pin number of the yellow led
      mcp1.pinMode(traffic_light[i].yellow, OUTPUT);                        // yellow led in output  (ID DCC+1/0)
      mcp1.digitalWrite(traffic_light[i].yellow, !MODE);                    // yellow led switch off
      traffic_light_jump++;                                             // the following traffic light 
      pin_jump+=3;                                                     // the following pin for three leds
          }
    else {                                                             // if two leds
         traffic_light[i].address = FIRST_ID_DCC + i;                  // its DCC ID
        pin_jump+=2;                                                   // the following pin for two leds
        }
  }
  test_at_start(); // cycle all lights ... 
  DCC.SetBasicAccessoryDecoderPacketHandler(BasicAccDecoderPacket_Handler, true);   // instanciate the DCC
  DCC.SetupDecoder( 0x00, 0x00, kDCC_INTERRUPT );                                   // its IT
  update_light = false;                                                             // no update  
  Serial.println("Let's go now ...");
}

/*************************************************************************
 * loop
 *************************************************************************/
 
void loop() {
  DCC.loop();                                              // Is there a DCC command ?
  if (update_light) {activation_traffic_light();}          // if yes, activation of traffic lights
  // if (millis()% 5000 == 0) {Serial.print(millis()); // uncomment if you want to see that the Arduino is still alive
    //  test_at_start(); // uncomment if you want more cycles to adjust
  }
  
