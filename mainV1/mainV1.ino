#include <LittleFS.h>

// PIN DEFINITIONS
#define FWD_PIN           25   // Relay 1 -> OONO FWD (Expel/Rise)
#define REV_PIN           26   // Relay 2 -> OONO REV (Suck/Sink)
#define SENSOR_PIN        34   // Pressure signal
#define BUILTIN_LED       2    
#define LOG_FILE          "/dive_log.csv"

// MISSION PARAMETERS
#define TARGET_DEPTH      2.5  
#define MAX_DEPTH_TARGET  4.8  
#define DEADZONE          0.1  
bool debug = false;

// Use unique names to avoid conflicts with Arduino's RISING keyword
enum MissionState { STATE_SINKING, STATE_RISING, STATE_HOVERING };
MissionState missionState = STATE_SINKING;
unsigned long lastLog = 0;

float getDepth() {
  int raw = analogRead(SENSOR_PIN);
  float voltage = raw * (3.3 / 4095.0);
  // Calibration: 4mA=0.66V, 20mA=2.64V
  float m = (voltage - 0.66) * (5.0 / 1.98); 
  return (m < 0) ? 0 : m;
}

void stopActuator() {
  digitalWrite(FWD_PIN, LOW);
  digitalWrite(REV_PIN, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(FWD_PIN, OUTPUT);
  pinMode(REV_PIN, OUTPUT);
  
  // Start with everything OFF
  stopActuator();

  if(!LittleFS.begin(true)) Serial.println("LittleFS Mount Fail");

  // --- 1. IMMEDIATE SYSTEM TEST ---
  Serial.println("--- SYSTEM TEST: CLEARING LIMIT SWITCHES ---");
  analogReadResolution(12);
  
  // TEST: Pull back first (REV) because you are currently fully extended
  Serial.println("Test: Retracting (REV)...");
  digitalWrite(REV_PIN, HIGH); 
  delay(1500); 
  digitalWrite(REV_PIN, LOW);

  delay(500);

  Serial.println("Test: Extending (FWD)...");
  digitalWrite(FWD_PIN, HIGH); 
  delay(1000); 
  digitalWrite(FWD_PIN, LOW);

  Serial.printf("Sensor Check: %.2fm\n", getDepth());

  // --- 2. 30 SECOND DELAY & LED FLASH ---
  Serial.println("Starting 30s Safety Countdown...");
  for (int i = 0; i < 30; i++) {
    int flashSpeed = (i < 20) ? 500 : 100; 
    digitalWrite(BUILTIN_LED, HIGH);
    delay(flashSpeed);
    digitalWrite(BUILTIN_LED, LOW);
    delay(flashSpeed);
    if (i % 5 == 0) Serial.printf("T-minus: %d\n", 30 - i);
  }
  Serial.println("MISSION INITIATED");
}

void logData(float d) {
  File file = LittleFS.open(LOG_FILE, FILE_APPEND);
  if(file) {
    file.printf("%lu,%.2f\n", millis()/1000, d);
    file.close();
  }
}

void exportCSV() {
  File file = LittleFS.open(LOG_FILE, FILE_READ);
  if(!file) {
    Serial.println("No log file.");
    return;
  }
  Serial.println("---CSV START---");
  Serial.println("Time(s),Depth(m)");
  while(file.available()) Serial.write(file.read());
  Serial.println("---CSV END---");
  file.close();
}

void loop() {
  float currentDepth = getDepth();
  if(debug == true){
      Serial.printf("Current Depth: %.2f meters\n", currentDepth);
  }

  // Log and check USB commands
  if (millis() - lastLog >= 1000) { logData(currentDepth); lastLog = millis(); }
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'E') exportCSV();
    if (c == 'D') { LittleFS.remove(LOG_FILE); Serial.println("Log Cleared"); }
  }

  // Buoyancy Control Logic
  switch (missionState) {
    case STATE_SINKING:
      if (currentDepth >= MAX_DEPTH_TARGET) { 
        stopActuator(); 
        missionState = STATE_RISING; 
      }
      else { 
        digitalWrite(REV_PIN, HIGH); // Suck water to sink
        digitalWrite(FWD_PIN, LOW); 
      }
      break;

    case STATE_RISING:
      if (currentDepth <= TARGET_DEPTH) { 
        stopActuator(); 
        missionState = STATE_HOVERING; 
      }
      else { 
        digitalWrite(FWD_PIN, HIGH); // Expel water to rise
        digitalWrite(REV_PIN, LOW); 
      }
      break;

    case STATE_HOVERING:
      if (currentDepth > (TARGET_DEPTH + DEADZONE)) { 
        digitalWrite(FWD_PIN, HIGH); // Too deep, push water out
        digitalWrite(REV_PIN, LOW); 
      }
      else if (currentDepth < (TARGET_DEPTH - DEADZONE)) { 
        digitalWrite(REV_PIN, LOW);
        digitalWrite(REV_PIN, HIGH); // Too shallow, suck water in
      }
      else { 
        stopActuator(); 
      }
      break;
  }
  delay(100);
}
