#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>

#define DEBOUNCER_TIMER 5
#define LCD_REFRESH_TIMER 50
#define DEFAULT_HYSTERESIS 2.5
#define SAFETY_TEMPERATURE 65.0

LiquidCrystal_I2C lcd(0x27,20,4);

// Pin alias ========================================================
int onButton = 2;
int offButton = 6;
int incTemp = 4;
int decTemp = 5;
int cooling = 8;
int heater = 9;

// Temp sensor initialization =======================================
OneWire oneWire(3);
DallasTemperature sensors(&oneWire);
unsigned long int DS18B20Millis = 0;


// Variable declaration =============================================
bool processOn = false;
int setTemp = 25;
float measuredTemp = 0;
int updateLCDTimer = 0;
int incButtonDebouncerCounter = 0;
bool incButtonDebouncerFlag = false;
int decButtonDebouncerCounter = 0;
bool decButtonDebouncerFlag = false;
float hysteresis = DEFAULT_HYSTERESIS;

void setup() {
  // Pin setup ======================================================
  pinMode(onButton, INPUT_PULLUP);
  pinMode(offButton, INPUT_PULLUP);
  pinMode(incTemp, INPUT_PULLUP);
  pinMode(decTemp, INPUT_PULLUP);
  pinMode(cooling, OUTPUT);
  pinMode(heater, OUTPUT);
  Serial.begin(9600);

  // Sensors initialization =========================================
  sensors.begin();

  // LCD initialization =============================================
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // PIN Initial values (output) ====================================
  digitalWrite(heater, LOW);  
  digitalWrite(cooling, LOW); 
}

/*  Relay output control functions. If hardware changes, please
    change here to adapt software
*/
void enableHeater() {
  digitalWrite(heater, HIGH);
}

void disableHeater() {
  digitalWrite(heater, LOW);
}

void enableCooling() {
  digitalWrite(cooling, HIGH);
}

void disableCooling() {
  digitalWrite(cooling, LOW);
}

// Main loop ========================================================
void loop() {
  
  // DS18B20 sensor reading control (reads every 1s) ================
  if (millis() - DS18B20Millis >= 1000ul)
  {
    //restart this TIMER
    DS18B20Millis = millis();

    measuredTemp = sensors.getTempCByIndex(0);

    Serial.print("Celsius temperature = ");
    Serial.println(measuredTemp);

    //start a new conversion request
    sensors.setWaitForConversion(false);
    sensors.requestTemperatures();
    sensors.setWaitForConversion(true);
  }

  // Turns process OFF if temp rises above SAFETY_TEMP ==============
  if (measuredTemp > SAFETY_TEMPERATURE) { 
    processOn = false;
  }

  // Turns process OFF when onButton is pressed =====================
  if (digitalRead(onButton) == LOW) { 
    processOn = true;
  }

  // Turns process OFF when offButton is pressed ====================
  if (digitalRead(offButton) == LOW) { 
    processOn = false;
  }

  // Increments temp if incTemp is pressed and temp < 65 ============
  if (digitalRead(incTemp) == LOW) { 
    incButtonDebouncerCounter++;
    if (incButtonDebouncerCounter > DEBOUNCER_TIMER
        && incButtonDebouncerFlag == false) {
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

  // Decreases temp if decTemp is pressed and temp > 25 =============
  if (digitalRead(decTemp) == LOW) { 
    decButtonDebouncerCounter++;
    if (decButtonDebouncerCounter > DEBOUNCER_TIMER 
        && decButtonDebouncerFlag == false) {
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

  // Executes ON/OFF control while process is ON ====================
  if (processOn) { 
    if (measuredTemp > SAFETY_TEMPERATURE) {
      processOn = false;
    }
    else {
      if (measuredTemp > setTemp + hysteresis) {
        enableCooling();
        disableHeater();
      }

      if (measuredTemp < setTemp - hysteresis) {
        disableCooling();
        enableHeater();
      }
    }
  }
  else { // Turns off heater, cools down to 30oC ====================
    disableHeater();
    if (measuredTemp > 30) {
      enableCooling();
    }
    else {
      disableCooling();
    }
  }

  // LCD Update control =============================================
  if (updateLCDTimer == LCD_REFRESH_TIMER) {
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
    if (digitalRead(heater) == 1) lcd.print("ON  ");
    else lcd.print("OFF ");
    lcd.print(" COOL=");
    if (digitalRead(cooling) == 1) lcd.print("ON  ");
    else lcd.print("OFF ");
    lcd.setCursor(0,3);
    lcd.print("HYST=");
    lcd.print(hysteresis);
    lcd.print("oC");
    lcd.setCursor(12,3);
    lcd.print("SUP=OFF");
  }
  else {
    updateLCDTimer++;
  }
}
