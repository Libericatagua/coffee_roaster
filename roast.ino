#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

// Pin definitions
const int CS_PIN = 15;
const int RELAY_PINS[] = {17, 16, 26, 25, 33};
const int RELAY_COUNT = sizeof(RELAY_PINS) / sizeof(RELAY_PINS[0]);

// Roasting parameters
const int TIME_FIRST_CRACK = 500;
const float ROAST_TYPE = 1.2;
const float START_TEMP = 185.0;
const int TARGET_TEMPS[] = {130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182, 184, 186, 188, 190, 192, 194, 196, 198, 200, 202, 204, 206, 208, 210, 212, 214, 216, 218, 220, 222, 224, 226, 228, 230, 232, 234, 236, 238, 240, 242, 244, 246, 248};

// Global variables
unsigned long startTime = 0;
unsigned long currentTime = 0;
float currentTemp = 0.0;
int phase = 1;
bool isRoastingComplete = false;

void setup() {
  Serial.begin(115200);
  initializeSD();
  initializePins();
  delay(1000);
}

void loop() {
  if (isRoastingComplete) return;

  switch (phase) {
    case 1:
      preheatingPhase();
      break;
    case 2:
      roastingPhase();
      break;
    case 3:
      coolingPhase();
      break;
  }
}

void initializeSD() {
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  if (SD.cardType() == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  Serial.println("SD card initialized successfully");
}

void initializePins() {
  SPI.begin();
  pinMode(CS_PIN, OUTPUT);
  for (int i = 0; i < RELAY_COUNT; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
  }
  setInitialRelayStates();
}

void setInitialRelayStates() {
  digitalWrite(RELAY_PINS[0], HIGH); // K1
  digitalWrite(RELAY_PINS[1], HIGH); // K2
  digitalWrite(RELAY_PINS[2], HIGH); // K3
  digitalWrite(RELAY_PINS[3], LOW);  // K4
  digitalWrite(RELAY_PINS[4], LOW);  // K5
}

void preheatingPhase() {
  Serial.println("Preheating phase");
  getTemperature();
  if (currentTemp <= START_TEMP) {
    digitalWrite(RELAY_PINS[1], HIGH); // K2
  } else {
    phase = 2;
    startTime = millis();
  }
  delay(3000);
}

void roastingPhase() {
  Serial.println("Roasting phase");
  delay(6000);
  getTemperature();
  updateTime();
  logData();

  int index = currentTime / 15;
  if (index < 60 && currentTemp >= TARGET_TEMPS[index]) {
    digitalWrite(RELAY_PINS[1], LOW); // K2
  } else if (currentTime > (TIME_FIRST_CRACK * ROAST_TYPE)) {
    phase = 3;
  } else {
    digitalWrite(RELAY_PINS[1], HIGH); // K2
  }
}

void coolingPhase() {
  Serial.println("Cooling phase");
  digitalWrite(RELAY_PINS[1], LOW); // K2
  digitalWrite(RELAY_PINS[3], HIGH); // K4

  for (int i = 0; i < 30; i++) {
    digitalWrite(RELAY_PINS[4], LOW); // K5
    delay(500);
    digitalWrite(RELAY_PINS[4], HIGH); // K5
    delay(500);
  }

  digitalWrite(RELAY_PINS[0], LOW); // K1
  digitalWrite(RELAY_PINS[2], LOW); // K3
  delay(200000);
  digitalWrite(RELAY_PINS[3], LOW); // K4
  isRoastingComplete = true;
  Serial.println("Roasting complete");
}

void getTemperature() {
  digitalWrite(CS_PIN, LOW);
  int rawTemp = SPI.transfer16(0x0000);
  digitalWrite(CS_PIN, HIGH);
  currentTemp = (rawTemp >> 3) * 0.25;
  Serial.println("Temperature: " + String(currentTemp) + "°C, Time: " + String(currentTime) + "s");
}

void updateTime() {
  currentTime = (millis() - startTime) / 1000;
}

void logData() {
  String dataString = String(currentTemp) + " " + String(startTime) + " " + String(currentTime);
  appendFile(SD, "/values.txt", dataString.c_str());
}

void appendFile(fs::FS &fs, const char * path, const char * message) {
  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.println(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}