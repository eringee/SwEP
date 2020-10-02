#include <SD.h>
#include <SPI.h>
#include <Servo.h>

//THESE AREAS NEED TO BE ADJUSTED FOR EVERY ROBOT UPLOAD WITH THE APPROPRIATE FILENAME AND ROBOT

//FILE ACCESS Example: B1chro.txt for chromatic test  OR B1main.txt for main song
char filename[] = "G2MAIN.TXT";    // b1 // g2 // m3 // q4 // x5 // z6 //  
char robot = 'G';

#define START_PIN 3       // receives activation signal from the RasPi/ESP8266
#define ROBOT_LIGHTS 20   //The white lights on the bells of the robot
#define TEST_LED 14       //The blue led on the PCB 

//Variables related to mode management: sleep, active, restart
#define INIT_WIFI 99
#define INIT_SLEEPMODE 0
#define SLEEPMODE 1
#define INIT_PERFMODE 2
#define PERFMODE 3

#define ROOMBACOMMAND 6
//////////////////////////

int activeState = INIT_SLEEPMODE;

IntervalTimer SDclocker;
IntervalTimer preshowClocker;
boolean Activate = false;

/////////////////////////////Variables for the fading lights         
long time;
int value;   
int periode = 4000;
int displace = 500; 
int fadeFlip = 1;
int fadeValue = 0;

/////////////////////////////////////Servo upkeep
Servo myservo1;
Servo myservo2;
Servo myservo3;

int servoDelay1 = 0;     //How long it takes for the servos to return after a hit
int servoDelay2 = 0;
int servoDelay3 = 0;
            
int returnTime = 6000;   //when to tell the servo to return

// servo positions for resting and hits
int resting1; int resting2; int resting3;
int hit1a; int hit2a; int hit3a;
int hit1b;int hit2b;int hit3b;  

//////////////////////////////SD card
File myFile;
const int chipSelect = 10;
String inputString = "";         // a string to hold incoming data
volatile boolean stringComplete = false;  // whether the string is complete
volatile boolean go = true; 
unsigned long timerSD = 0; // int for timer between commmands
int x1;  //commandBuffpointer
int y;  //stringPositionPointer
long SDcommand[16];

/////////////////////////////Variables for Roomba command
int roombaDelay = 0;     //how long to wait before executing another command
uint8_t option = 0; 
uint8_t data = 0;

int roombaBuffer [16]; // this might need to be bigger at some point?
int roombaBuffpointer = 0;

int checksum = 0;
char command;
byte commandBuffer[24]; 
int length;
int x = 0;  //related to roomba commands. Needs to be a global variable (?)

//////////////////////iRobot Create Open Interface commands:
#define rmbaSTART 128
#define rmbaFULL 132
// rmbaDRiVE_DIRECT: command, right velocity, left velocity (-500 - 500mm/s)
#define rmbaDRiVE_DIRECT 145
// rmbaSTREAM: command, number of packets, packetIDs
#define rmbaSTREAM 148
#define rmbaQUERY_LIST 149
// rmbaSTREAM_ENABLE 1 enables stream
#define rmbaSTREAM_ENABLE 150
// rmbaSCRIPT: Script command, number of bytes in script, script 
#define rmbaSCRIPT 152
#define rmbaPLAY_SCRIPT 153
// rmbaWAIT_TIME command, time in s/10
#define rmbaWAIT_TIME 155
// rmbaDRIVE: Drive command, velocity(+/-1500mm/s) hi lo, radius(+-2000mm) hi lo
#define rmbaDRIVE 137
// rmbaWAIT_ANGLE: command, angle in degrees hi lo (CW is negative)
#define rmbaWAIT_ANGLE 157

//scripts for obstacle avoidance
byte turnright[] = { 
  152, 13, 137, 0, 200, 0, 0, 157, 0, 90, 137, 0, 1, 1, 100 };
byte turnleft[] = { 
  152, 13, 137, 255, 56, 0, 0, 157, 255, 166, 137, 0, 1, 1, 100 };
byte turnaround[] = { 
  152, 13, 137, 0, 200, 0, 0, 157, 0, 180, 137, 0, 1, 1, 100 };

// [Velocity high byte] [Velocity low byte] [Radius high byte] [Radius low byte]
byte leftforward[5] = {rmbaDRIVE, highByte(100), lowByte(100), highByte(129), lowByte(129) };
byte rightforward[5] = {rmbaDRIVE, highByte(100), lowByte(100), highByte(-129), lowByte(-129) };
byte moveforward[5] = {rmbaDRIVE, highByte(80), lowByte(80), highByte(0x7FFF), lowByte(0x7FFF) };

byte stopmoving[5] = {rmbaDRIVE, highByte(0), lowByte(0), highByte(1), lowByte(1) };
byte spinleft[5] = {rmbaDRIVE, highByte(60), lowByte(60), highByte(1), lowByte(1) };
byte spinright[5] = {rmbaDRIVE, highByte(60), lowByte(60), highByte(0xFFFF), lowByte(0xFFFF) };

IntervalTimer roombaDataClocker;
IntervalTimer servoReturnClocker;

// roombaTick is called by roombaDataClocker every millisecond

boolean roombaDelay100 = false;
boolean roombaDelayIsZero = true; // first time

boolean servoReturn25 = false;    //25 millis before servo returns to neutral position
boolean servoReturnIsZero = true; // first time

void roombaDataTick() {
  if (roombaDelay != 0) {
    roombaDelay--;
    roombaDelayIsZero = (0 == roombaDelay);
  }
  roombaDelay100 = (1 == roombaDelay%100); // so we don't set it when it's zero
}

/*
void servoTick() {
  if (servoDelay != 0) {
    servoDelay--;
    servoDelayIsZero = (0 == servoDelay);
  }
  servoDelay25 = (1 == servoDelay%25); // so we don't set it when it's zero
}*/

void setup()
{  
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  //  while (!Serial);
  // Serial.print("Serial began");

  randomSeed(analogRead(16));  //seed random values

  pinMode(TEST_LED, OUTPUT);   //test LED
  pinMode(ROBOT_LIGHTS, OUTPUT);    //main lights in the robot bells
  pinMode(START_PIN, INPUT_PULLUP);   //activation pin
  
  //Setup SD card    
  if (!SD.begin(chipSelect)) {
    //Serial.println("initialization failed!");
    digitalWrite(TEST_LED, HIGH);
    delay(1000);
    digitalWrite(TEST_LED, LOW);
    return;
  }
  inputString.reserve(200);  //space for writing to Teensy from SD
                     
  //clear unused pins for the sake of power conservation
  pinMode(16, OUTPUT); 
  pinMode(17, OUTPUT);
  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(21, OUTPUT);
  pinMode(22, OUTPUT);

////////////////////////////////Startup Roomba routine
  //Open serial communications for Roomba robot
  //setup for ESP8266 communications
  //Serial3.begin(9600);
  
  delay(6000);
  Serial2.begin(57600);
  //Serial.println("Serial2 on"); 
  delay(1000);  
  
  Serial2.write(rmbaSTART); // Passive Mode
  //Serial.println("Passive Mode");
  delay(100);
  Serial2.write(rmbaFULL); // Full Control
  //Serial.println("Full Control");
  delay(1000);

  roombaDataClocker.begin(roombaDataTick, 1000); // millisecond clock
 // servoReturnClocker.begin(servoTick, 1000); // ms clock 
  
/*
  commandBuffer [0]=148;//stream robot sensors
  commandBuffer [1]=2;  //two sensor packets
  commandBuffer [2]=7;  // left and right bumper sensors
  commandBuffer [3]=17; // infrared byte
  
  Serial2.write(commandBuffer, 4); 
  Serial.println("commence streaming");    
  delay(100);
*/

Serial.println ("Roomba Setup Complete");

switch (robot) {
   //resting 1 = rightmost (servo marked 3)
   case 90: // ROBOT Z VERIFIED
       resting1 = 90; //rightmost  
       resting2 = 80;
       resting3 = 90;
 
       hit1a = 55;
       hit2a = 55;
       hit3a = 55;
 
       hit1b = 120;
       hit2b = 110;
       hit3b = 120; 
    break;
    
    case 81: //ROBOT Q - VERIFIED
       resting1 = 99; //LEFTmost  
       resting2 = 97;
       resting3 = 80;
 
       hit1a = 70;
       hit2a = 65;
       hit3a = 60;
 
       hit1b = 130;
       hit2b = 135;
       hit3b = 110; 
     break;
     
     case 88: //ROBOT X
             
       resting1 = 76; //center 
       resting2 = 72;
       resting3 = 85;

       hit1a = 60;
       hit2a = 50;
       hit3a = 65;
 
       hit1b = 110;
       hit2b = 110;
       hit3b = 115;
     break;
     
     case 66: //ROBOT B
       resting1 = 87; 
       resting2 = 87;
       resting3 = 87;
       
       hit1a = 65;
       hit2a = 65;
       hit3a = 65;
       
       hit1b = 115;
       hit2b = 115;
       hit3b = 115;
     break;
     
     case 71: //ROBOT G - verified
       resting1 = 77; 
       resting2 = 90;
       resting3 = 77;
 
       hit1a = 62;
       hit2a = 70;
       hit3a = 62;
 
       hit1b = 110;
       hit2b = 120;
       hit3b = 105;
     break;
     
     case 77: //ROBOT M - VERIFIED
       resting1 = 83; //RIGHTmost  
       resting2 = 77;
       resting3 = 73;
 
       hit1a = 60;
       hit2a = 65;
       hit3a = 60;
 
       hit1b = 100;
       hit2b = 100;
       hit3b = 100; 
     break;
     
    default: 
      Serial.println("Non valid robot designation");
      digitalWrite(TEST_LED, HIGH);
      preshowClocker.end();
    break;
  }
}

///////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {

//This section tests for wireless connectivity.
//Test LED was lit during setup.  If ESP connects to internet than the led is turned off.

  if (activeState == INIT_WIFI){
    digitalWrite(TEST_LED, HIGH);  // blue light comes on to indicate network isn't joined yet
    if (Serial3.available()) {
      int inByte = Serial3.read();
      Serial.write(inByte);
      if (inByte==66) {   // if inByte = "B" (ESP says "Bunny" every 15 seconds connected to WiFi)
        digitalWrite(TEST_LED, LOW);
        activeState = INIT_SLEEPMODE; //proceed to sleepmode setup
      }
    }
  }

//Sleepmode setup: read the first directive in the SD card
  if (activeState == INIT_SLEEPMODE) { 
    myFile = SD.open(filename);  //test to see if you can open the SD file
    while (!myFile) {
      Serial.println("This file isn't opening");
      digitalWrite(TEST_LED, HIGH);
      delay(1000);
      digitalWrite(TEST_LED, LOW);
      delay(1000);
    }     
  while(!stringComplete) getSDstring();  //read string
  timerSD = (SDcommand[0]*1000);
  go = false;
  //Serial.println("Command loaded");
  digitalWrite(ROBOT_LIGHTS, LOW);
  activeState = SLEEPMODE;
  }
  
  ///////////////////////////// SLEEP MODE
  if (activeState == SLEEPMODE) {
    Serial.println("sleep mode");
    time = millis();   
    value = 128+127*cos(2*PI/periode*(displace-time));  //fade lights
    analogWrite(ROBOT_LIGHTS, value);
    delay(10);
  ////////WAIT FOR ACTIVATION SIGNAL
    detectButton();  // "test" installation button (active when LOW)   
  }
  
  ///////////////////////////// PERFORMANCE MODE
  //SETUP ROUTINE
  if (activeState == INIT_PERFMODE) {   //button is pressed, installation is active.     
    Serial.println("waiting for video to start!");
    
    delay(3500);          //change this value to affect the time waited after activation
    
    myservo1.attach(4);  //attach servos to pin 4-5-6
    delay(5);  
    myservo2.attach(5);
    delay(5);
    myservo3.attach(6);
    delay(5);
    myservo1.write(resting1);   // send servos to resting mode
    delay(5);
    myservo2.write(resting2);
    delay(5);
    myservo3.write(resting3);
    delay(5);
      
    SDclocker.begin(executeCommand, timerSD);  // Begin SD commands
    activeState = PERFMODE;   //exit setup
  }
  ///////////////////////////////////////////////////////////
  /////////////// Performance mode loop
  ///////////////////////////////////////////////////////////

  if (activeState == PERFMODE) {
    servoReturn();  // servos return to neutral position
    lights();      
  /*
   * 
  ////ROOMBA MOVEMENT COMMANDS /////////////////////
  
    static int selector;  //determines which action the Roomba performs
  
    if (roombaDelayIsZero) {  //if Roomba is ready for next command
      
      roombaDelayIsZero = false;
      int rotateDirection = random(0, 10);       
      switch (selector) {
          
          //The robot will rotate left or right, followed by a waiting period
          case 0:         //robot rotates
            if (rotateDirection%2 == 0) {
            Serial.println("SpinLeft");
            Serial2.write(spinleft, 5);
            }
            else {
              Serial.println("SpinRight");
              Serial2.write(spinright, 5);
            }
            break;
            
          case 1:          //robot stops
            Serial.println("Stopped");
            Serial2.write(stopmoving, 5);
            break;
        }
      selector = (selector+1)%2;
      noInterrupts(); // because roombaDelay is a multibyte variable we won't want it to change while we mess with it
      roombaDelay = random(3000, 5000); // random value between 1 and 5 seconds
      interrupts();
    }
   
    if (roombaDelay100) { // every 100ms
      roombaDelay100 = false;
      commandBuffer [0]=rmbaQUERY_LIST;// stream robot sensors. 149 = send values once
      commandBuffer [1]=2;   //two sensor packets
      commandBuffer [2]=22;  //voltage in battery in mV
      commandBuffer [3]=23;  //current in mA
      Serial2.write(commandBuffer, 4);
    }
    
    while (Serial2.available() >= 4) {
      int mVh = Serial2.read();
      int mVl = Serial2.read();
      int mAh = Serial2.read();
      int mAl = Serial2.read();
      unsigned int mV = (mVh<<8)|mVl;
      int mA = (mAh<<8)|mAl;
      Serial.print(mV, DEC);
      Serial.print("mV ");
      mA = 65536 - mA; // <— not sure you need this
      Serial.print(mA, DEC);
      Serial.print("mA ");
      Serial.println(",");
    }*/
    
    if (!stringComplete) getSDstring();  //get next command from SD
    else if (stringComplete) {
        
      //Test SD command to see if file is over
      if (go) { // test to see if we are still executing a servo command 
          
        if (SDcommand[0] == 254) {  // last command
          Serial2.write(stopmoving, 5);  //stop Roomba
          
          //send servos back to neutral position
          myservo1.write(resting1);
          myservo2.write(resting2);
          myservo3.write(resting3);
          for (int timer = returnTime; timer < 0; timer--) {
            digitalWrite(TEST_LED, HIGH);
          }
          //detach servos
          myservo1.detach();
          delay(5);  
          myservo2.detach();
          delay(5);
          myservo3.detach();
          delay(5); 
          digitalWrite(ROBOT_LIGHTS, HIGH);  //turn on light when finished
          myFile.close();
          delay(10000);    // pause at end of performance           
          resetVars();     // start back at the beginning
        }
          
        else {
          timerSD = (SDcommand[0]*1000);
          SDclocker.begin(executeCommand, timerSD);
          go = false;  // flag that we are in the middle of command and not to shut down installation
        }
      }  
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////FUNCTIONS///////////////////////////////////////////////

void executeCommand() {
  
  if (SDcommand[1] == ROOMBACOMMAND) {  // communication with Roomba base: puts command into buffer
    // Serial.println("roomba");
    x = 0;
    length = SDcommand[3];
    for (int i=4; i <= length+3;i++) {   //whoa this only works with certain commands....
      Serial2.write(SDcommand[i]);
      //  Serial.print(SDcommand[i]);            
      commandBuffer[x]=(SDcommand[i]);
      x++;
      if (x >= 4) x = 0;
      go = true;
    }
  }  

  if ((SDcommand[1] >= 2) && (SDcommand[1] <= 4)) {  // betwen 2-4 is a servo hit
    
    if (SDcommand[1] == 4){    // tell servo1 to go to position in variable 'pos'
      if (SDcommand[3] == 1) myservo1.write(hit1a);
      if (SDcommand[3] == 2) myservo1.write(hit1b);
      servoDelay1 = returnTime;
    }

    if (SDcommand[1] == 3) {
      if (SDcommand[3] == 1) myservo2.write(hit2a);
      if (SDcommand[3] == 2) myservo2.write(hit2b); 
      servoDelay2 = returnTime;
    }

    if (SDcommand[1] == 2) { 
      if (SDcommand[3] == 1) myservo3.write(hit3a);
      if (SDcommand[3] == 2) myservo3.write(hit3b);   
      servoDelay3 = returnTime;
    }
    fadeValue=1400;  // lights will activate and fade according to this variable
    go = true;  
  }

  if (SDcommand[1] == 5) {      //activate lights - possibly not used
    fadeValue=(SDcommand[3]*3);
    go = true;
  } 
  if (SDcommand[1] == 7) go = true;  //end of SD line
  stringComplete = false;
  SDclocker.end();
} 

void servoReturn() {     //return servos to resting position
  if (servoDelay1 > 1) {
    servoDelay1--;
    if (servoDelay1 == 1) {
      myservo1.write(resting1);
    }
  }
  if (servoDelay2 > 1) {
    servoDelay2--;
    if (servoDelay2 == 1) {
      myservo2.write(resting2);
    }
  }
  if (servoDelay3 > 1) {
    servoDelay3--;
    if (servoDelay3 == 1) {
      myservo3.write(resting3);
    }
  }
}

void runScript(byte*command, int numCommands) {  // send script commands to roomba
  clearScript();
  for (int i = 0; i < numCommands; i++) {
    Serial2.write(command[i]);   //coms with Roomba base 
    delay(2);
  }
}

void clearScript() {   //clear Roomba base buffer

  byte nothing[] = {
    152, 0   };
  Serial2.write(nothing[0]);
  delay(1);
  Serial2.write(nothing[1]);
  delay(1);
}

void getSDstring() {  //get string from SD card
  
  char inChar = (char)myFile.read(); // add it to the inputString:
  inputString += inChar;
  if (inChar == '\n') {

    //   Serial.println(inputString); 
    int firstSpace = inputString.indexOf(" ");
    int lastSpace = inputString.lastIndexOf(" ");
    int newline = inputString.indexOf("\n"); 
    int secondSpace = firstSpace;

    String nextNumberString = inputString.substring(0, firstSpace);

    SDcommand[0] = nextNumberString.toInt();

    analogWrite(ROBOT_LIGHTS, fadeValue);

    for (x1 = 1; firstSpace < lastSpace; x1++) { 
      firstSpace = secondSpace;
      secondSpace = inputString.indexOf(32, firstSpace+1);
      nextNumberString = inputString.substring(firstSpace+1, secondSpace);
      SDcommand[x1] = nextNumberString.toInt();
    }
    nextNumberString = inputString.substring(lastSpace+1, newline);
    SDcommand[x1] = nextNumberString.toInt();

    /*   for(int z = 0; z < x1-1; z++) {
     Serial.print("SDcommand ");
     Serial.print(z);
     Serial.print(" = ");
     Serial.println(SDcommand[z]);
     } */
     
    inputString = ""; // clear the string
    stringComplete = true;
  }
}

void detectButton() {   //detect activation button in installation
  //LOW signal or Serial3 inByte "1" activates button
  
  /*if (Serial3.available()) {
    int inByte = Serial3.read();
    Serial.write(inByte);
    if (inByte==49) {     // 1
      digitalWrite(TEST_LED, HIGH);
      delay(1000);
      digitalWrite(TEST_LED, LOW);
      activeState = INIT_PERFMODE;
    }
  }*/
  
  if(digitalRead(START_PIN) == LOW) {    //debounce mechanism
    digitalWrite(ROBOT_LIGHTS, HIGH);
    delay(1000);
    activeState = INIT_PERFMODE;
  }
}

void lights() {
  analogWrite(ROBOT_LIGHTS, fadeValue);
  
  if (fadeFlip == 1) {   //flips fade direction
    fadeValue--;
    fadeFlip = 2;
  }
  if (fadeFlip == 0) fadeFlip = 1;
  if (fadeFlip == 2) fadeFlip = 0;
}

void resetVars() {
  activeState = INIT_SLEEPMODE;    
  while(Serial.available()){Serial.read();}
  Activate = false;

  servoDelay1 = 0;     //How long it takes for the servos to return after a hit
  servoDelay2 = 0;
  servoDelay3 = 0;
  stringComplete = false;  // whether the string is complete
  go = true; 
  timerSD = 0; // int for timer between commmands

  roombaDelay = 0;     //variable for how long to wait before executing another command
  option = 0; 
  data = 0;
  roombaBuffpointer = 0;

  checksum = 0;
  x = 0;   // relates to Roomba commands

  roombaDelay100 = false;
  roombaDelayIsZero = true; // first time
}
