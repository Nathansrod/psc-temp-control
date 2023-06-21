#include <DS18B20.h>

// Pin alias
int onButton = 2;
int offButton = 3;
int incTemp = 4;
int decTemp = 5;
int fan = 8;
int heater = 9;
int pump = 10
DS18B20 ds(11);

void setup() {
  // Pin setup
  pinMode(onButton, INPUT);
  pinMode(offButton, INPUT);
  pinMode(incTemp, INPUT);
  pinMode(decTemp, INPUT);
  pinMode(fan, OUTPUT);
  pinMode(heater, OUTPUT);
  pinMode(pump, OUTPUT);
  Serial.begin(9600);

  // Variable declaration
  bool processOn = false;
  int setTemp = 25;
  int measuredTemp = 30;

}

void loop() {
  measuredTemp = ds.getTempC();

  if (digitalRead(onButton)) {
    processOn = true;
  }

  if (digitalRead(offButton)) {
    processOn = false;
  }

  if (digitalRead(incTemp)) {
    if (setTemp < 65) {
      setTemp++;
    }
  }

  if (digitalRead(decTemp)) {
    if (setTemp > 25) {
      setTemp--;
    }
  }

  if (processOn) {

  }
  else {
    digitalWrite(heater, LOW);
    if (measuredTemp > 30) {
      digitalWrite(fan, HIGH);
      digitalWrite(pump, HIGH);
    }
    else {
      digitalWrite(fan, LOW);
      digitalWrite(pump, LOW);
    }
  }
}
