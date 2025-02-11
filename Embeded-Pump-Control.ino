#include <millisDelay.h>
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

//Tank Variables
#define upperTankTrigPin 25 // also A pin in sensor
#define upperTankEchoPin 26 // also B pin in sensor

#define lowerTankTrigPin 12 // already connected
#define lowerTankEchoPin 13

#define touchPin 4

#define PumpRelayPin 23


BluetoothSerial SerialBT;


const int upperTankNormalReadingInterval = 30000;
const int upperTankFillingReadingInterval = 500;

const int lowerTankFillingReadingInterval = 1000;
const int lowerTankNormalReadingInterval = 500;


const int upperTankFullWaterDistance = 23;
const int upperTankEmptyWaterDistance = 50;

const int lowerTankEmptyWaterDistance = 76;
const int lowerTankFullWaterDistance = 56;
//

millisDelay uppertankNormalReadingDelay;
millisDelay uppertankFillingReadingDelay;

millisDelay lowertankNormalReadingDelay;
millisDelay lowertankFillingReadingDelay;


bool isUpperTankEmpty = false;
bool isPumpOn = false;
bool isLowerTankEmpty = false;

bool toTurnPumpOn = false;

int upperTankWaterLevel = 0;
int lowerTankWaterLevel = 0;

String message = "";
char incomingChar;

void setup() {
  Serial.begin(115200);

  pinMode(upperTankTrigPin, OUTPUT);
  pinMode(lowerTankTrigPin, OUTPUT);
  pinMode(upperTankEchoPin, INPUT);
  pinMode(lowerTankEchoPin, INPUT);

  pinMode(PumpRelayPin, OUTPUT);


  digitalWrite(PumpRelayPin, HIGH);


  upperTankWaterLevel = getAvgUpperTankWaterLevel(3);
  lowerTankWaterLevel = getAvgLowerTankWaterLevel(3);

  SerialBT.begin("PumpControl");
  turnPumpOff();
  uppertankNormalReadingDelay.start(upperTankNormalReadingInterval);
}

void loop() {
  if (SerialBT.available()){
    char incomingChar = SerialBT.read();
    if (incomingChar != '\n'){
      message += String(incomingChar);
    }
    else{
      message = "";
    } 
  }

  if (message =="stats"){
    showStats();
  }

  if(!isUpperTankEmpty && uppertankNormalReadingDelay.justFinished()){
    uppertankNormalReadingDelay.repeat();
    upperTankWaterLevel = getAvgUpperTankWaterLevel(3);
    if (upperTankWaterLevel >= upperTankEmptyWaterDistance){
      log("Upper Tank Empty");
      isUpperTankEmpty = true;
      lowertankNormalReadingDelay.start(lowerTankNormalReadingInterval);
      uppertankFillingReadingDelay.start(upperTankFillingReadingInterval);
      uppertankNormalReadingDelay.stop();
    }
  }
  else if (isUpperTankEmpty && uppertankFillingReadingDelay.justFinished()) {
    uppertankFillingReadingDelay.repeat();
    upperTankWaterLevel = getAvgUpperTankWaterLevel(3);
    if (upperTankWaterLevel <= upperTankFullWaterDistance){
      log("Upper Tank Full");
      isUpperTankEmpty = false;
      uppertankFillingReadingDelay.stop();
      lowertankNormalReadingDelay.stop();
      uppertankNormalReadingDelay.start(upperTankNormalReadingInterval);

    }    
  }

  if(isUpperTankEmpty && !isLowerTankEmpty && lowertankNormalReadingDelay.justFinished()){
    lowertankNormalReadingDelay.repeat();
    lowerTankWaterLevel = getAvgLowerTankWaterLevel(3);
    if(lowerTankWaterLevel >= lowerTankEmptyWaterDistance){   // True if lower tank is empty
      isLowerTankEmpty = true;
      lowertankFillingReadingDelay.start(lowerTankFillingReadingInterval);
      lowertankNormalReadingDelay.stop();
      log("Lower Tank Empty");
      //turnPumpOff();
    }
    
  }
  else if (isUpperTankEmpty && isLowerTankEmpty && lowertankFillingReadingDelay.justFinished()){
    lowertankFillingReadingDelay.repeat();
    lowerTankWaterLevel = getAvgLowerTankWaterLevel(3);
    if(lowerTankWaterLevel <= lowerTankFullWaterDistance){
      isLowerTankEmpty = false;
      lowertankFillingReadingDelay.stop();
      lowertankNormalReadingDelay.start(lowerTankNormalReadingInterval);
      log("Lower Tank Full");
    }
    
  }
  if(isUpperTankEmpty){
    if(!isLowerTankEmpty && !isPumpOn){
      turnPumpOn();
    }
    if(isLowerTankEmpty && isPumpOn){
      turnPumpOff();
    }
  }
  if(!isUpperTankEmpty && isPumpOn){
    turnPumpOff();
  }

  delay(10);
}


int getAvgUpperTankWaterLevel(int times){
  int i = 1;
  float totalDistance = 0;
  while(i<=times){
    totalDistance += getUpperTankDistance();
    i++;
  }
  String msg = "Upper tank water level is: ";
  msg.concat(totalDistance/times);
  log(msg);
  return totalDistance/times;
}


float getUpperTankDistance(){
  delay(200);
  digitalWrite(upperTankTrigPin, LOW);
  delayMicroseconds(5);
 
  digitalWrite(upperTankTrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(upperTankTrigPin, LOW);
  long duration;
  duration = pulseIn(upperTankEchoPin, HIGH);
  float calculatedDistance = duration*0.034/2;
  return calculatedDistance;
}


int getAvgLowerTankWaterLevel(int times){
  int i = 1;
  float totalDistance = 0;

  while(i<=times){
    totalDistance += getLowerTankDistance();
    i++;
  }
  String msg = "Lower tank water level is: ";
  msg.concat(totalDistance/times);
  log(msg);
  return totalDistance/times;
}


float getLowerTankDistance(){
  delay(200);
  digitalWrite(lowerTankTrigPin, LOW);
  delayMicroseconds(5);
 
  digitalWrite(lowerTankTrigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(lowerTankTrigPin, LOW);
  long duration;
  duration = pulseIn(lowerTankEchoPin, HIGH);
  float calculatedDistance = duration*0.034/2;
  //Serial.println(calculatedDistance);
  return calculatedDistance;
}

void showStats(){
  log("The Stats Are: ");
  getAvgLowerTankWaterLevel(3);
  getAvgUpperTankWaterLevel(3);
  if(isPumpOn){
    log("The pump is on");
  }
  else{
    log("The pump is off");
  }
}

void turnPumpOn(){
  log("Turning Pump On");
  isPumpOn = true;
  digitalWrite(PumpRelayPin, LOW);
}

void turnPumpOff(){
  log("Turning Pump Off");
  isPumpOn = false;
  digitalWrite(PumpRelayPin, HIGH);
}

void log(String msg){
  Serial.println(msg);
  SerialBT.println(msg);
}