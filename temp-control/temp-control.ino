#include <DS18B20.h>
#include <LiquidCrystal_I2C.h>

#define DEBOUNCER_TIMER 5
#define LCD_REFRESH_TIMER 50

LiquidCrystal_I2C lcd(0x27,20,4);

// Pin alias
int onButton = 2;
int offButton = 3;
int incTemp = 5;
int decTemp = 4;
int fan = 8;
int heater = 9;
int pump = 10;

// Temp sensor initialization
DS18B20 ds(11);

// Variable declaration
bool processOn = false;
int setTemp = 25;
float measuredTemp = 0;
int updateLCDTimer = 0;
int incButtonDebouncerCounter = 0;
bool incButtonDebouncerFlag = false;
int decButtonDebouncerCounter = 0;
bool decButtonDebouncerFlag = false;

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
  Serial.println(measuredTemp);

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
    incButtonDebouncerCounter++;
    if (incButtonDebouncerCounter > DEBOUNCER_TIMER && incButtonDebouncerFlag == false) {
      if (setTemp < 65) {
        setTemp++;
        incButtonDebouncerFlag = true;
      }
    }
  }
  else {
    incButtonDebouncerCounter = 0;
    incButtonDebouncerFlag = false;
  }

  if (digitalRead(decTemp) == LOW) { // Decreases temp if decTemp is pressed and temp > 25
    decButtonDebouncerCounter++;
    if (decButtonDebouncerCounter > DEBOUNCER_TIMER && decButtonDebouncerFlag == false) {
      if (setTemp > 25) {
        setTemp--;
        decButtonDebouncerFlag = true;
      }
    }
  }
  else {
    decButtonDebouncerCounter = 0;
    decButtonDebouncerFlag = false;
  }

  if (processOn) { // Executes control while process is ON
    digitalWrite(pump, HIGH);
  }
  else { // Turns off heater, and keeps pump and fan on to cool down if temp > 30 and process is OFF
    digitalWrite(heater, LOW);
    digitalWrite(pump, LOW);
    // if (measuredTemp > 30) {
    //   digitalWrite(fan, HIGH);
    //   digitalWrite(pump, HIGH);
    // }
    // else {
    //   digitalWrite(fan, LOW);
    //   digitalWrite(pump, LOW);
    // }
  }

  if (updateLCDTimer == LCD_REFRESH_TIMER) {
    // LCD Update
    updateLCDTimer = 0;
    lcd.setCursor(0,0);
    if (processOn) {
      lcd.print("TEMPERATURE CTRL ON ");
    }
    else {
      lcd.print("TEMPERATURE CTRL OFF");
    }
    lcd.setCursor(0,1);
    lcd.print("PV=");
    lcd.print(measuredTemp);
    lcd.print("oC");
    lcd.setCursor(11,1);
    lcd.print("SP=");
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
