// TEZNET Access Point //  TeZ 2019

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <Metro.h>
#include <String.h>
#include "wifisettings.h"
#define ALIVE_ACK_MS 5000
Metro alive = Metro(10000);

unsigned long previousMillis = 0;     
unsigned long preMilli = 0; 

OSCErrorCode error;
int actcmd = 0;

//#define DIGITAL_PIN  2
#define DEBUG true

void setup() {
//  pinMode(DIGITAL_PIN, OUTPUT);
  Serial.begin(9600); 
  startWIFI();  
}

void loop() {

    state_loop();
    osc_message_pump();  
}

/////////////////////////////////////
/// READ OSC MESSAGES ///
void osc_message_pump() {  
  OSCMessage in;
  int size;

  if( (size = Udp.parsePacket()) > 0)
  {
    // parse incoming OSC message
    while(size--) {
      in.fill( Udp.read() );
    }

    if(!in.hasError()) {
     in.route("/robot/active", on_robotactive); 
      
    }else {
      error = in.getError();
      Serial.print("osc message input error: ");
      Serial.println(error);
    }
  }
}

/////////////////////////////////////
void state_loop() {
  // send alive ACK message
  if(alive.check()) {
    alive.interval(15000);
    Serial.println("Bunny");
  }
}

/////////////////////////////////////
void on_ack(OSCMessage &msg, int addrOffset) {
  
  if(DEBUG) {Serial.println(">> OSC ACK"); }

}

/////////////////////////////////
void on_robotactive(OSCMessage &msg, int addrOffset) {

    if(msg.isInt(0)){
      actcmd = msg.getInt(0);
      Serial.println(actcmd);
      if (actcmd == 1) {
        //disconnect wifi for 20 mins
        WiFi.disconnect();
        Serial.println("Wifi disconnected");
        delay(1000*60*20); //if you want to disconnect WiFi for 20 mins..
        startWIFI(); 
      }
    }
}


/////////////////////////////////////
void sendOSCip(){

  WIP[0]=myIP[0];
  WIP[1]=myIP[1];
  WIP[2]=myIP[2];
  WIP[3]=myIP[3];

  OSCMessage out("/wemos/ip"); // send to server "dest" specified in wifisettings.h
   out.add(WIP[0]);
   out.add(WIP[1]);
   out.add(WIP[2]);
   out.add(WIP[3]);
   Udp.beginPacket(dest, txport);
   out.send(Udp);
   Udp.endPacket();
   out.empty();

}

/////////////////////////////////////////
void StartUDP(){

   Udp.begin(rxport);
    
    Serial.println("Starting UDP");
    Serial.print("Local port: ");
    Serial.println(Udp.localPort());
    
    device_id = WID;

    delay(1000);
    
    sendOSCip();
}

/////////////////////////////////////////
void startWIFI(){

   // init network
    Serial.print("Connecting to ");
    Serial.println(ssid);

   // Start network procedures 
    WiFi.begin(ssid, pass);

    int secz=0;
    
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.print(".");
      if(secz<9){
        secz++;
        }else{
          secz=0; 
          Serial.println("_");
        }      
    }
    
    Serial.println("");
    Serial.println("WiFi connected");  
    //Serial.println("IP address: ");
    thisip = WiFi.localIP();
    //Serial.println( thisip );

    Udp.begin(rxport);

//    blinkled(3);

}


/////////////////////////////////////////
/*
void blinkled(int rep){

      for(int i=1; i<=rep; i++){
      digitalWrite(DIGITAL_PIN, HIGH);
      delay(500);
      digitalWrite(DIGITAL_PIN, LOW);
      delay(500); 
    }
}
*/

void CheckWiFi(){
  //connect wifi if not connected
  if (WiFi.status() != WL_CONNECTED) {
      delay(1);
      // startWIFI();
      return;
      }
}
