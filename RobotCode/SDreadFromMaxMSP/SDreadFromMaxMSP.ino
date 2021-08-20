#include <SD.h>
#include <SPI.h>

File myFile;

int command[16];
int x; // commandBuffpointer
int y; // stringPositionPointer

void setup() {
  
  Serial.begin(9600);
  const int chipSelect = 10;
   
  if (!SD.begin(chipSelect)) {
    Serial.println("initialization failed!");
    return;
  }
//  Serial.println("initialization done.");
  
  // open the file. 
  myFile = SD.open("B1MAIN.TXT", FILE_WRITE);  //filenames must be 8 chars or less
    
  while (!myFile) {
//    Serial.println("This file isn't opening");
    delay(1000);
  }
}

void loop() {

  delay(2); 
  String stringOne = "";
  char c = 0;
  while(Serial.available() > 0) {
    c = Serial.read();
    stringOne.concat(c);
    if (c == 59) break;    // 59 = semicolon 
  }
  // if a semicolon is sent, create a newline
  if (c == 59) {   
  
    int firstSpace = stringOne.indexOf(32);
    int lastSpace = stringOne.lastIndexOf(32);
    int semicolon = stringOne.indexOf(59);
  
    int secondSpace = firstSpace;

    String nextNumberString = stringOne.substring(0, firstSpace);
    command[0] = nextNumberString.toInt();
    
    if (command[0] == 255) {    // this should be the last command in the coll
      myFile.close();
      Serial.println("Finished.  Remove power.");
      while(Serial) ;
    }
    else {
      for (x = 1; secondSpace < lastSpace; x++) { 
        firstSpace = secondSpace;
        secondSpace = stringOne.indexOf(32, firstSpace+1);
        nextNumberString = stringOne.substring(firstSpace+1, secondSpace);
        command[x] = nextNumberString.toInt();
      }
  
      for(int z = 0; z < x; z++) {
        Serial.print(command[z]);
        myFile.print(command[z]);
        myFile.print(" ");
      }
      Serial.println();
      myFile.println();
    }
  }
}
