#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Pin assignments
const int buzzerPin = 9;
const int led1Pin = 6;
const int led2Pin = 7;
const int buttonPin = 2;

// System state flags
volatile bool buttonPressed = false;
int systemState = 0; // 0 - active, 1 - alert, 2 - standby

// LCD with I2C
LiquidCrystal_I2C lcd(0x27, 16, 2);

// MPU6050 sensor
const int MPU_ADDR = 0x68;
int16_t accX, accY, accZ;
int16_t accX_ref = 0, accY_ref = 0, accZ_ref = 0;

const int threshold = 15000; // Vibration threshold to trigger alert

void setup() {
  pinMode(led1Pin, OUTPUT);
  pinMode(led2Pin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);

  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.backlight();

  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); // PWR_MGMT_1 register
  Wire.write(0);    // Wake up MPU6050
  Wire.endTransmission(true);

  calibreazaSistemul();
  playStartupTone();

  attachInterrupt(digitalPinToInterrupt(buttonPin), onButtonPress, FALLING); // Detect button pressed
}

void loop() {
  if (buttonPressed) {
    buttonPressed = false;

    if (systemState == 1) {
      stopAlarm();      // Stop alert
      systemState = 2;  // Go to standby
    } else if (systemState == 2) {
      calibreazaSistemul(); // Recalibrate
      playStartupTone();
      systemState = 0;      // Back to active monitoring
    }
  }

  if (systemState == 0) {
    readAccel();

    int deltaX = abs(accX - accX_ref);
    int deltaY = abs(accY - accY_ref);
    int deltaZ = abs(accZ - accZ_ref);

    // Send data to Serial Plotter
    Serial.print(accX);
    Serial.print("\t");
    Serial.print(accY);
    Serial.print("\t");
    Serial.println(accZ);

    if (deltaX > threshold || deltaY > threshold || deltaZ > threshold) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ALERTA CUTREMUR!");

      Serial.println("ALERTA CUTREMUR!");

      Serial.print("Valori actuale → ");
      Serial.print("X: "); Serial.print(accX);
      Serial.print(" | Y: "); Serial.print(accY);
      Serial.print(" | Z: "); Serial.println(accZ);

      Serial.print("Diferente fata de valorile de referinta → ");
      Serial.print("ΔX: "); Serial.print(deltaX);
      Serial.print(" | ΔY: "); Serial.print(deltaY);
      Serial.print(" | ΔZ: "); Serial.println(deltaZ);

      systemState = 1;
    }
  }

  if (systemState == 1) {
    tone(buzzerPin, 1300);
    delay(200);
    noTone(buzzerPin);
    delay(200);

    digitalWrite(led1Pin, HIGH);
    digitalWrite(led2Pin, LOW);
    delay(200);
    digitalWrite(led1Pin, LOW);
    digitalWrite(led2Pin, HIGH);
    delay(200);
  }

  if (systemState == 2) {
    // Standby mode: do nothing
  }
}

void stopAlarm() {
  noTone(buzzerPin);
  digitalWrite(led1Pin, LOW);
  digitalWrite(led2Pin, LOW);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistem oprit");
  Serial.println(">> Alarma oprita");
  playShutdownTone();
}

void onButtonPress() {
  buttonPressed = true;
}

void readAccel() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B); // ACCEL_XOUT_H
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 6, true);

  accX = Wire.read() << 8 | Wire.read();
  accY = Wire.read() << 8 | Wire.read();
  accZ = Wire.read() << 8 | Wire.read();
}

void calibreazaSistemul() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Calibrare sistem");

  long sumX = 0, sumY = 0, sumZ = 0;

  for (int i = 5; i >= 1; i--) {
    lcd.setCursor(0, 1);
    lcd.print("Start in: ");
    lcd.print(i);
    lcd.print("  ");
    
    for (int j = 0; j < 20; j++) {  // 20 readings per second
      readAccel();
      sumX += accX;
      sumY += accY;
      sumZ += accZ;
      delay(50);
    }
  }

  accX_ref = sumX / 100;
  accY_ref = sumY / 100;
  accZ_ref = sumZ / 100;

  Serial.print(">> Calibrare → X: ");
  Serial.print(accX_ref);
  Serial.print(" | Y: "); Serial.print(accY_ref);
  Serial.print(" | Z: "); Serial.println(accZ_ref);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sistem activat.");
  delay(1500);
  lcd.clear();
}

void playStartupTone() {
  tone(buzzerPin, 880, 150);
  delay(200);
  tone(buzzerPin, 1200, 150);
  delay(200);
  noTone(buzzerPin);
}

void playShutdownTone() {
  tone(buzzerPin, 600, 200);
  delay(250);
  noTone(buzzerPin);
}
