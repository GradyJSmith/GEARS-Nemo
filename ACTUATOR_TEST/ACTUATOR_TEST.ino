// PIN DEFINITIONS
#define FWD_PIN 25   // Relay 1 -> OONO FWD (Expel water)
#define REV_PIN 26   // Relay 2 -> OONO REV (Suck water)

void setup() {
  Serial.begin(115200);
  
  pinMode(FWD_PIN, OUTPUT);
  pinMode(REV_PIN, OUTPUT);
  
  // Start with everything OFF
  digitalWrite(FWD_PIN, LOW);
  digitalWrite(REV_PIN, LOW);

  Serial.println("--- MANUAL BUOYANCY ENGINE TEST ---");
  Serial.println("Commands:");
  Serial.println(" [F] - Forward (Expel Water / Rise)");
  Serial.println(" [B] - Backward (Suck Water / Sink)");
  Serial.println(" [S] - STOP");
  Serial.println("-----------------------------------");
}

void loop() {
  if (Serial.available() > 0) {
    char cmd = Serial.read();
    
    // Convert to uppercase so 'f' and 'F' both work
    if (cmd >= 'a' && cmd <= 'z') cmd -= 32; 

    if (cmd == 'F') {
      Serial.println("Action: FORWARD (Expelling)");
      digitalWrite(REV_PIN, LOW);
      digitalWrite(FWD_PIN, HIGH);
    } 
    else if (cmd == 'B') {
      Serial.println("Action: BACKWARD (Sucking)");
      digitalWrite(FWD_PIN, LOW);
      digitalWrite(REV_PIN, HIGH);
    } 
    else if (cmd == 'S') {
      Serial.println("Action: STOP");
      digitalWrite(FWD_PIN, LOW);
      digitalWrite(REV_PIN, LOW);
    }
  }
}
