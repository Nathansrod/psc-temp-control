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
  int measuredTemp = 0;

}

void loop() {
  measuredTemp = ds.getTempC();

  if (measuredTemp > 70) { // Turns process OFF if temp rises above 70
    processOn = false;
  }

  if (digitalRead(onButton)) { // Turns process OFF when onButton is pressed
    processOn = true;
  }

  if (digitalRead(offButton)) { // Turns process OFF when offButton is pressed
    processOn = false;
  }

  if (digitalRead(incTemp)) { // Increments temp if incTemp is pressed and temp < 65
    if (setTemp < 65) {
      setTemp++;
    }
  }

  if (digitalRead(decTemp)) { // Decreases temp if decTemp is pressed and temp > 25
    if (setTemp > 25) {
      setTemp--;
    }
  }



  if (processOn) { // Executes control while process is ON
    if (measuredTemp > setTemp) {
      digitalWrite(pump, HIGH);
      digitalWrite(pump)
    }
  }
  else { // Turns off heater, and keeps pump and fan on to cool down if temp > 30 and process is OFF
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
