#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <Modbusino.h>

#define DEBOUNCER_TIMER 5
#define LCD_REFRESH_TIMER 50
#define DEFAULT_HYSTERESIS 0.5
#define SAFETY_TEMPERATURE 60
#define EMPTY_ALL false

LiquidCrystal_I2C lcd(0x27,20,4);

// Modbus settings ==================================================
ModbusinoSlave modbusino_slave(1);
uint16_t tabreg[7];

// Pin alias ========================================================
int onButton = 2;
int offButton = 6;
int incTemp = 5;
int decTemp = 4;
int fan = 8;
int heater = 9;
int pump = 10;

// Temp sensor initialization =======================================
OneWire oneWire(3);
DallasTemperature sensors(&oneWire);
unsigned long int DS18B20Millis = 0;

// Variable declaration =============================================
bool processOn = false;
int setTemp = 25;
float measuredTemp = 0;
float previousTemp = 0;
int updateLCDTimer = 0;
int incButtonDebouncerCounter = 0;
bool incButtonDebouncerFlag = false;
int decButtonDebouncerCounter = 0;
bool decButtonDebouncerFlag = false;
float hysteresis = DEFAULT_HYSTERESIS;
int scadaStatus = 0;

void setup() {
  // Modbus setup ===================================================
  modbusino_slave.setup(9600);

  // Pin setup ======================================================
  pinMode(onButton, INPUT_PULLUP);
  pinMode(offButton, INPUT_PULLUP);
  pinMode(incTemp, INPUT_PULLUP);
  pinMode(decTemp, INPUT_PULLUP);
  pinMode(fan, OUTPUT);
  pinMode(pump, OUTPUT);
  pinMode(heater, OUTPUT);

  // Sensors initialization =========================================
  sensors.begin();

  // LCD initialization =============================================
  lcd.init();
  lcd.backlight();
  lcd.clear();
  if (EMPTY_ALL) {
    lcd.print("EMPTYING...");
  }

  // PIN Initial values (output) ====================================
  digitalWrite(heater, HIGH);  
  digitalWrite(fan, HIGH); 
}

/*  Relay output control functions. If hardware changes, please
    change here to adapt software
*/
void enableHeater() {
  digitalWrite(heater, LOW);
}

void disableHeater() {
  digitalWrite(heater, HIGH);
}

void enableCooling() {
  digitalWrite(fan, LOW);
  digitalWrite(pump, HIGH);
}

void disableCooling() {
  digitalWrite(fan, HIGH);
  digitalWrite(pump, LOW);
}

// Main loop ========================================================
void loop() {
  if (!EMPTY_ALL) {
    // DS18B20 sensor reading control (reads every 1s) ================
    if (millis() - DS18B20Millis >= 1000ul)
    {
      //restart this TIMER
      DS18B20Millis = millis();

      previousTemp = measuredTemp;
      measuredTemp = sensors.getTempCByIndex(0);
      if (measuredTemp < 0) { // Discard dirty temp read ==============
        measuredTemp = previousTemp;
      }

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
      if (digitalRead(heater) == 0) lcd.print("ON  ");
      else lcd.print("OFF ");
      lcd.print(" COOL=");
      if (digitalRead(fan) == 0) lcd.print("ON  ");
      else lcd.print("OFF ");
      lcd.setCursor(0,3);
      lcd.print("HYST=");
      lcd.print(hysteresis);
      lcd.print("oC");
      lcd.setCursor(12,3);
      lcd.print("SUP=");
      lcd.print(scadaStatus);
    }
    else {
      updateLCDTimer++;
    }

    tabreg[0] = measuredTemp * 100;
    tabreg[1] = setTemp;
    tabreg[2] = digitalRead(heater);
    tabreg[3] = digitalRead(fan);
    tabreg[4] = processOn ? 1 : 0;
    tabreg[5] = hysteresis * 10;
    tabreg[6] = digitalRead(pump);

    scadaStatus = modbusino_slave.loop(tabreg, 7);
  }
  else {
    digitalWrite(pump, HIGH);
  }
}