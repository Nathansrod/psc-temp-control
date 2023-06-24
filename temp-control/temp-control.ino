#include <DS18B20.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27,20,4);

// Pin alias
int onButton = 2;
int offButton = 3;
int incTemp = 5;
int decTemp = 4;
int fan = 8;
int heater = 9;
int pump = 10;
DS18B20 ds(11);

// Variable declaration
bool processOn = false;
int setTemp = 25;
int measuredTemp = 0;
int updateLCDTimer = 0;

void setup() {
  // Pin setup
  pinMode(onButton, INPUT_PULLUP);
  pinMode(offButton, INPUT_PULLUP);
  pinMode(incTemp, INPUT_PULLUP);
  pinMode(decTemp, INPUT_PULLUP);
  pinMode(fan, OUTPUT);
  pinMode(heater, OUTPUT);
  pinMode(pump, OUTPUT);
  Serial.begin(9600);

  // LCD initialization
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

void loop() {
  measuredTemp = ds.getTempC();

  if (measuredTemp > 70) { // Turns process OFF if temp rises above 70
    processOn = false;
  }

  if (digitalRead(onButton) == LOW) { // Turns process OFF when onButton is pressed
    processOn = true;
  }

  if (digitalRead(offButton) == LOW) { // Turns process OFF when offButton is pressed
    processOn = false;
  }

  if (digitalRead(incTemp) == LOW) { // Increments temp if incTemp is pressed and temp < 65
    if (setTemp < 65) {
      setTemp++;
    }
  }

  if (digitalRead(decTemp) == LOW) { // Decreases temp if decTemp is pressed and temp > 25
    if (setTemp > 25) {
      setTemp--;
    }
  }

  if (processOn) { // Executes control while process is ON
    
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

  if (updateLCDTimer == 100) {
    // LCD Update
    updateLCDTimer = 0;
    lcd.clear();
    lcd.setCursor(0,0);
    if (processOn) {
      lcd.print("TEMPERATURE CTRL ON");
    }
    else {
      lcd.print("TEMPERATURE CTRL OFF");
    }
    lcd.setCursor(0,1);
    lcd.print("TMP(PV/SP)=");
    lcd.print(measuredTemp);
    lcd.print("oC/");
    lcd.print(setTemp);
    lcd.print("oC");
    lcd.setCursor(0,2);
    lcd.print("HEAT=");
    lcd.print(digitalRead(heater));
    lcd.print(" PUMP=");
    lcd.print(digitalRead(pump));
    lcd.print(" FAN=");
    lcd.print(digitalRead(fan));
  }
  else {
    updateLCDTimer++;
  }
}
