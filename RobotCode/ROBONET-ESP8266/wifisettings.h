///////////////////////////////////////////
// Code by TeZ 2019

const char* ssid = "SwEP-private";
const char* pass = "emotional";
const char* mySsid = "ROBONET";
bool con = false;
 
 IPAddress ip(192,168,1,102);
 IPAddress gateway(192,168,1,1);
 IPAddress subnet(255,255,255,0);
 IPAddress myIP(0,0,0,0);
///////////////////////////////////////////

WiFiUDP Udp;                               // A UDP instance to let us send and receive packets over UDP
const IPAddress dest(192, 168, 1, 255);    // AP_ROBOT broadcast address
const unsigned int rxport = 54321;         // port to receive OSC from outside
const unsigned int txport = 12345;         // remote port to send OSC packets (set this in dest)

IPAddress thisip;
int device_id = -1;
int WID = 23; 
int WIP[]={0,0,0,0};
