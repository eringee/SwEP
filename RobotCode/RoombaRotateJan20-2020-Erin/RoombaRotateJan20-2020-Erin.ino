
int testLed = 14;   //The blue LED on the PCB for testing

/////////////////////////////Variables for Roomba command
int roombaDelay = 0;     //variable for how long to wait before executing another command
uint8_t option = 0; 
uint8_t data = 0;

int roombaBuffer [16]; // this might need to be bigger at some point?
int roombaBuffpointer = 0;

int checksum = 0;
char command;
byte commandBuffer[24];
int length;
int x = 0; 

int scriptSelect = 0;  //variable for selecting whether robot is moving or not


// iRobot Create Open Interface commands:
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


///scripts for the Roomba
byte leftforward[5] = {rmbaDRIVE, highByte(200), lowByte(200), highByte(129), lowByte(129) };
byte rightforward[5] = {rmbaDRIVE, highByte(200), lowByte(200), highByte(-129), lowByte(-129) };
byte moveforward[5] = {rmbaDRIVE, highByte(150), lowByte(150), highByte(0x7FFF), lowByte(0x7FFF) };

byte stopmoving[5] = {rmbaDRIVE, highByte(0), lowByte(0), highByte(1), lowByte(1) };
byte spinleft[5] = {rmbaDRIVE, highByte(144), lowByte(100), highByte(1), lowByte(1) };
byte spinright[5] = {rmbaDRIVE, highByte(144), lowByte(100), highByte(0xFFFF), lowByte(0xFFFF) };

IntervalTimer roombaDelayClocker;

// roombaTick is called by roombaDelayClocker every millisecond

boolean roombaDelay100 = false;
boolean roombaDelayIsZero = true; // first time
void roombaTick() {
  if (roombaDelay != 0) {
    roombaDelay--;
    roombaDelayIsZero = (0 == roombaDelay);
  }
  roombaDelay100 = (1 == roombaDelay%100); // so we don't set it when it's zero
}
  
void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  //  while (!Serial);
  // Serial.print("Serial began");
  
  randomSeed(analogRead(16)); 
  pinMode(testLed, OUTPUT);   
  
  delay(4000); //allow the Roomba to boot

  Serial2.begin(57600);  //initiate Roomba serial
  //Serial.println("Serial2 on"); 
  delay(1000);  
  
  Serial2.write(rmbaSTART); // Mandatory passive mode initiation
  //Serial.println("Passive Mode");
  delay(100);
  Serial2.write(rmbaFULL); // full control mode initiation
  //Serial.println("Full Control");
  delay(1000);

  roombaDelayClocker.begin(roombaTick, 1000); // millisecond clock

  Serial.println ("Setup complete");
}

void loop() {
  static int selector;

  if (roombaDelayIsZero) {
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
    roombaDelay = random(1000, 5000); // random value between 3 and 8 seconds
    interrupts();
  }
  //query sensor packets
  if (roombaDelay100) { // every 100ms
    roombaDelay100 = false;
    commandBuffer [0]=rmbaQUERY_LIST;// stream robot sensors. 149 = send values once
    commandBuffer [1]=2;   //two sensor packets
    commandBuffer [2]=22;  //voltage in battery in mV
    commandBuffer [3]=23;  //current in mA
    Serial2.write(commandBuffer, 4);
  }
  
  while ( Serial2.available() >= 4) {
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
  }
}
