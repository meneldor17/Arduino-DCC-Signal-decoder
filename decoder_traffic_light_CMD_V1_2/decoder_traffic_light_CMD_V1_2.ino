/*
 * Decoder for traffic light with three or two leds on ARDUINO NANO
 * 
 * MAny thanks to (i hope i do not forget anybody)
 * - Minabay library https://github.com/MynaBay/DCC_Decoder
 * - Informations and main code https://www.locoduino.org/spip.php?article161 
 * - i used a simple connection to DCC (see circuit in github
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
#define NB_TRAFFIC_LIGHT  10           //  ssuming this is tricolor 

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



#define INSTALLEDMCPNB 2 // number of installed MCP
#define DELAY1 3000 // used in initial test 
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

//  DCC

#include <DCC_Decoder.h>                 // Minabay library
#define kDCC_INTERRUPT    0             // pin 2 receives DCC interrupts
int previous_address = 0;               // avoids multiple DCC addresses
int previous_position = 2;              // avoids multiple DCC orders
volatile boolean update_light;          // set if an update should be processed after DCC control

// traffic light

#define BICOLOR  8                     // 8 traffic lights with two leds per MCP
#define TRICOLOR 5                     // 5 traffic lights with three leds per MCP
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

/**************************************************************
 * method called if a request is made by the DCC
 * 
 *******************************************************************/
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
                      mcps[(int) i/TRICOLOR].digitalWrite(traffic_light[i].green,MODE);         // switch on green
                      mcps[(int) i/TRICOLOR].digitalWrite(traffic_light[i].red,!MODE);          // switch off red
                      if ( NB_TRAFFIC_LIGHT == TRICOLOR){mcps[(int) i/TRICOLOR].digitalWrite(traffic_light[i].yellow,!MODE);} // switch off yellow
                      #ifdef CONSOLE
                        Serial.print("activation -> traffic light");Serial.print(i+1);Serial.println(" : Green led");
                      #endif
                      break;}
        case RED : {                                                  // if red led
                      mcps[(int) i/TRICOLOR].digitalWrite(traffic_light[i].green,!MODE);      // switch off green
                      mcps[(int) i/TRICOLOR].digitalWrite(traffic_light[i].red,MODE);         // switch on red
                      if ( NB_TRAFFIC_LIGHT == TRICOLOR){mcps[(int) i/TRICOLOR].digitalWrite(traffic_light[i].yellow,!MODE);} // switch off yellow
                      #ifdef CONSOLE
                        Serial.print("activation -> traffic light");Serial.print(i+1);Serial.println(" : Red led");
                      #endif
                     break;}
         case YELLOW : {                                                  // if yellow led
                      mcps[(int) i/TRICOLOR].digitalWrite(traffic_light[i].green,!MODE);         // switch off green 
                      mcps[(int) i/TRICOLOR].digitalWrite(traffic_light[i].red,!MODE);           // switch off red
                      mcps[(int) i/TRICOLOR].digitalWrite(traffic_light[i].yellow,MODE);         // switch on yellow
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


// MCP Setup overall on MCP 1
  
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
  
