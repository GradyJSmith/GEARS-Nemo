#include <LittleFS.h>

// PIN DEFINITIONS
#define RELAY_RISE_PIN    25   // Expel water (Make Light)
#define RELAY_SINK_PIN    26   // Suck water (Make Heavy)
#define SENSOR_PIN        34   // Pressure data line
#define LOG_FILE          "/dive_log.csv"

// MISSION PARAMETERS
#define TARGET_DEPTH      2.5  // Meters
#define MAX_DEPTH_TARGET  4.8  // "All the way down"
#define DEADZONE          0.1  // 10cm window

enum State { SINKING, RISING, HOVERING };
State missionState = SINKING;
unsigned long lastLog = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize LittleFS
  if(!LittleFS.begin(true)) {
    Serial.println("LittleFS Mount Failed");
    return;
  }

  pinMode(RELAY_RISE_PIN, OUTPUT);
  pinMode(RELAY_SINK_PIN, OUTPUT);
  digitalWrite(RELAY_RISE_PIN, HIGH); // Off (Active Low)
  digitalWrite(RELAY_SINK_PIN, HIGH); // Off
  
  analogReadResolution(12);
  Serial.println("System Ready. Send 'E' to export, 'D' to delete log.");
}

float getMeters() {
  int raw = analogRead(SENSOR_PIN);
  float voltage = raw * (3.3 / 4095.0);
  float m = (voltage - 0.66) * (5.0 / 1.98); 
  return (m < 0) ? 0 : m;
}

void logEntry(float depth) {
  File file = LittleFS.open(LOG_FILE, FILE_APPEND);
  if(file) {
    file.printf("%lu,%.2f\n", millis()/1000, depth);
    file.close();
  }
}

void exportCSV() {
  File file = LittleFS.open(LOG_FILE, FILE_READ);
  if(!file) return;
  Serial.println("---START---");
  Serial.println("Seconds,Depth_m");
  while(file.available()) Serial.write(file.read());
  Serial.println("---END---");
  file.close();
}

void loop() {
  float currentDepth = getMeters();

  // Logging every second
  if (millis() - lastLog >= 1000) {
    logEntry(currentDepth);
    lastLog = millis();
  }

  // USB Commands
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'E') exportCSV();
    if (c == 'D') { LittleFS.remove(LOG_FILE); Serial.println("Deleted."); }
  }

  // Buoyancy Logic
  switch (missionState) {
    case SINKING:
      if (currentDepth >= MAX_DEPTH_TARGET) missionState = RISING;
      else { digitalWrite(RELAY_SINK_PIN, LOW); digitalWrite(RELAY_RISE_PIN, HIGH); }
      break;
    case RISING:
      if (currentDepth <= TARGET_DEPTH) missionState = HOVERING;
      else { digitalWrite(RELAY_RISE_PIN, LOW); digitalWrite(RELAY_SINK_PIN, HIGH); }
      break;
    case HOVERING:
      if (currentDepth > (TARGET_DEPTH + DEADZONE)) {
        digitalWrite(RELAY_RISE_PIN, LOW); digitalWrite(RELAY_SINK_PIN, HIGH);
      } else if (currentDepth < (TARGET_DEPTH - DEADZONE)) {
        digitalWrite(RELAY_SINK_PIN, LOW); digitalWrite(RELAY_RISE_PIN, HIGH);
      } else {
        digitalWrite(RELAY_RISE_PIN, HIGH); digitalWrite(RELAY_SINK_PIN, HIGH);
      }
      break;
  }
}
