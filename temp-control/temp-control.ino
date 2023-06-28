#include <DS18B20.h>
#include <LiquidCrystal_I2C.h>

#define DEBOUNCER_TIMER 5
#define LCD_REFRESH_TIMER 50

LiquidCrystal_I2C lcd(0x27,20,4);

// Pin alias
int onButton = 2;
int offButton = 6;
int incTemp = 4;
int decTemp = 5;
int cooling = 8;
int heater = 9;

// Temp sensor initialization
DS18B20 ds(3);

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
  pinMode(cooling, OUTPUT);
  pinMode(heater, OUTPUT);
  Serial.begin(9600);

  // LCD initialization
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // PIN Initial values (output)
  digitalWrite(heater, LOW);  
  digitalWrite(cooling, LOW); 
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
    digitalWrite(cooling, HIGH); // LOW = ON on relay output pins
  }
  else { // Turns off heater, and keeps cooling on to cool down if temp > 30 and process is OFF
    digitalWrite(heater, LOW);
    digitalWrite(cooling, LOW);
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
    lcd.print(" COOLING=");
    lcd.print(digitalRead(cooling));
  }
  else {
    updateLCDTimer++;
  }
}
