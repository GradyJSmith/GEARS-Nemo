/* --- CONFIGURATION: RELAYS FOR SYRINGE --- */
#define RELAY_EXPEL_WATER 25  // Extend actuator (Make Light / Rise)
#define RELAY_SUCK_WATER  26  // Retract actuator (Make Heavy / Sink)
#define SENSOR_PIN        34  // Pressure sensor signal

/* --- MISSION PARAMETERS --- */
#define MAX_DEPTH_TARGET  4.8  // "All the way down" depth in meters
#define HOVER_TARGET      2.5  // Target depth to maintain
#define DEADZONE          0.1  // 10cm window to prevent constant clicking

/* --- STATE TRACKING --- */
enum MissionState { SINKING_TO_BOTTOM, RISING_TO_HOVER, STABILIZING };
MissionState mission = SINKING_TO_BOTTOM;

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_EXPEL_WATER, OUTPUT);
  pinMode(RELAY_SUCK_WATER, OUTPUT);
  
  // Start with relays OFF (Assuming Active Low)
  digitalWrite(RELAY_EXPEL_WATER, HIGH);
  digitalWrite(RELAY_SUCK_WATER, HIGH);
  
  analogReadResolution(12);
}

float readMeters() {
  int raw = analogRead(SENSOR_PIN);
  float voltage = raw * (3.3 / 4095.0);
  float meters = (voltage - 0.66) * (5.0 / 1.98); 
  return (meters < 0) ? 0 : meters;
}

void allStop() {
  digitalWrite(RELAY_EXPEL_WATER, HIGH);
  digitalWrite(RELAY_SUCK_WATER, HIGH);
}

void makeLight() { // Expel water to rise
  digitalWrite(RELAY_EXPEL_WATER, LOW);
  digitalWrite(RELAY_SUCK_WATER, HIGH);
}

void makeHeavy() { // Suck water to sink
  digitalWrite(RELAY_EXPEL_WATER, HIGH);
  digitalWrite(RELAY_SUCK_WATER, LOW);
}

void loop() {
  float currentDepth = readMeters();

  switch (mission) {
    case SINKING_TO_BOTTOM:
      if (currentDepth >= MAX_DEPTH_TARGET) {
        allStop();
        mission = RISING_TO_HOVER;
      } else {
        makeHeavy(); // Pull syringe in to sink
      }
      break;

    case RISING_TO_HOVER:
      if (currentDepth <= HOVER_TARGET) {
        allStop();
        mission = STABILIZING;
      } else {
        makeLight(); // Push syringe out to rise
      }
      break;

    case STABILIZING:
      // Stay at 2.5m
      if (currentDepth > (HOVER_TARGET + DEADZONE)) {
        makeLight(); // Too deep, push water out
      } else if (currentDepth < (HOVER_TARGET - DEADZONE)) {
        makeHeavy(); // Too shallow, suck water in
      } else {
        allStop();
      }
      break;
  }

  // Debugging output
  Serial.printf("Depth: %.2fm | Mode: %d\n", currentDepth, mission);
  delay(200); 
}
