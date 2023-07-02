#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <Modbus.h>
#include <ModbusSerial.h>

// Constant declaration =============================================
#define DEBOUNCER_TIMER 5
#define LCD_REFRESH_TIMER 50
#define DEFAULT_HYSTERESIS 0.5
#define SAFETY_TEMPERATURE 60.0
#define EMPTY_ALL true

// Modbus register offest ===========================================
#define FAN_STATUS 0
#define HEATER_STATUS 0
#define PUMP_STATUS 0
#define PROCESS_COIL 0
#define TEMP_PV_IREG 0
#define TEMP_SP_HREG 0
#define HYST_HREG 0

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

// LCD declaration ==================================================
LiquidCrystal_I2C lcd(0x27,20,4);

// Variable declaration =============================================
bool isProcessOn = false;
int setTemp = 25;
float measuredTemp = 0;
float previousTemp = 0;
int updateLCDTimer = 0;
int incButtonDebouncerCounter = 0;
bool incButtonDebouncerFlag = false;
int decButtonDebouncerCounter = 0;
bool decButtonDebouncerFlag = false;
float hysteresis = DEFAULT_HYSTERESIS;
ModbusSerial mb(Serial, 1, SERIAL_8N1);

void setup() {
  // Modbus configuration ===========================================
  mb.config(9600);
  mb.addIsts(FAN_STATUS);
  mb.addIsts(HEATER_STATUS);
  mb.addIsts(PUMP_STATUS);
  mb.addCoil(PROCESS_COIL);
  mb.addIreg(TEMP_PV_IREG);
  mb.addHreg(TEMP_SP_HREG);
  mb.addHreg(HYST_HREG, DEFAULT_HYSTERESIS);

  // Pin setup ======================================================
  pinMode(onButton, INPUT_PULLUP);
  pinMode(offButton, INPUT_PULLUP);
  pinMode(incTemp, INPUT_PULLUP);
  pinMode(decTemp, INPUT_PULLUP);
  pinMode(fan, OUTPUT);
  pinMode(pump, OUTPUT);
  pinMode(heater, OUTPUT);
  Serial.begin(9600);

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
    mb.task();
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

      Serial.print("Celsius temperature = ");
      Serial.println(measuredTemp);

      //start a new conversion request
      sensors.setWaitForConversion(false);
      sensors.requestTemperatures();
      sensors.setWaitForConversion(true);
    }

    // Turns process OFF if temp rises above SAFETY_TEMP ==============
    if (measuredTemp > SAFETY_TEMPERATURE) { 
      isProcessOn = false;
    }

    // Turns process OFF when onButton is pressed =====================
    if (digitalRead(onButton) == LOW) { 
      isProcessOn = true;
      mb.setCoil(PROCESS_COIL, true);
    }

    // Turns process OFF when offButton is pressed ====================
    if (digitalRead(offButton) == LOW) { 
      isProcessOn = false;
      mb.setCoil(PROCESS_COIL, false);
    }

    // Increments temp if incTemp is pressed and temp < 65 ============
    if (digitalRead(incTemp) == LOW) { 
      incButtonDebouncerCounter++;
      if (incButtonDebouncerCounter > DEBOUNCER_TIMER
          && incButtonDebouncerFlag == false) {
        if (setTemp < 65) {
          setTemp++;
          incButtonDebouncerFlag = true;
          mb.setHreg(TEMP_SP_HREG, setTemp);
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
          mb.setHreg(TEMP_SP_HREG, setTemp);
        }
      }
    }
    else {
      decButtonDebouncerCounter = 0;
      decButtonDebouncerFlag = false;
    }

    // Executes ON/OFF control while process is ON ====================
    if (isProcessOn) { 
      if (measuredTemp > SAFETY_TEMPERATURE) {
        isProcessOn = false;
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
      if (isProcessOn) {
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
      lcd.print("SUP=OFF");
    }
    else {
      updateLCDTimer++;
    }

    // Modbus information exchange ==================================
    mb.Ists(FAN_STATUS, !digitalRead(fan));
    mb.Ists(HEATER_STATUS, !digitalRead(heater));
    mb.Ists(PUMP_STATUS, digitalRead(pump));
    mb.Ireg(TEMP_PV_IREG, measuredTemp);
    hysteresis = mb.Hreg(HYST_HREG);
    setTemp = mb.Hreg(TEMP_SP_HREG);
    isProcessOn = mb.Coil(PROCESS_COIL);
     
  }
  else {
    digitalWrite(pump, HIGH);
  }
}
