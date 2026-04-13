#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>

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

// -------------------------------------------------------
// PROFILE SETTINGS
// Set how many full down-and-back-up cycles to run
// -------------------------------------------------------
#define PROFILE_COUNT     3

// WiFi AP credentials — connect to this hotspot to view data
const char* AP_SSID     = "FloatLogger";
const char* AP_PASSWORD = "divedata";   // min 8 chars, or set to "" for open

WebServer server(80);
int profilesCompleted = 0;

// Use unique names to avoid conflicts with Arduino's RISING keyword
enum MissionState { STATE_SINKING, STATE_RISING, STATE_HOVERING, STATE_DONE };
MissionState missionState = STATE_SINKING;
unsigned long lastLog = 0;

// -------------------------------------------------------
// EXISTING FUNCTIONS — UNTOUCHED
// -------------------------------------------------------

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

// -------------------------------------------------------
// WIFI + WEBSERVER FUNCTIONS
// -------------------------------------------------------

// Reads the CSV from LittleFS and builds an HTML table + raw download link
void handleRoot() {
  File file = LittleFS.open(LOG_FILE, FILE_READ);

  String html = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Float Dive Log</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Barlow:wght@400;600&display=swap');
    :root {
      --bg: #0a0f1a;
      --panel: #111827;
      --border: #1e3a5f;
      --accent: #00c8ff;
      --accent2: #00ffb3;
      --text: #cde8ff;
      --muted: #4a6fa5;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      background: var(--bg);
      color: var(--text);
      font-family: 'Barlow', sans-serif;
      min-height: 100vh;
      padding: 2rem;
    }
    header {
      border-bottom: 1px solid var(--border);
      padding-bottom: 1rem;
      margin-bottom: 2rem;
    }
    header h1 {
      font-family: 'Share Tech Mono', monospace;
      color: var(--accent);
      font-size: 1.6rem;
      letter-spacing: 0.1em;
    }
    header p { color: var(--muted); font-size: 0.85rem; margin-top: 0.3rem; }
    .stats {
      display: flex; gap: 1.5rem; flex-wrap: wrap; margin-bottom: 2rem;
    }
    .stat-card {
      background: var(--panel);
      border: 1px solid var(--border);
      border-radius: 8px;
      padding: 1rem 1.5rem;
      min-width: 140px;
    }
    .stat-card .label { font-size: 0.72rem; color: var(--muted); text-transform: uppercase; letter-spacing: 0.08em; }
    .stat-card .value { font-family: 'Share Tech Mono', monospace; font-size: 1.5rem; color: var(--accent2); margin-top: 0.25rem; }
    .download-btn {
      display: inline-block;
      margin-bottom: 1.5rem;
      padding: 0.5rem 1.2rem;
      background: transparent;
      border: 1px solid var(--accent);
      color: var(--accent);
      font-family: 'Share Tech Mono', monospace;
      font-size: 0.85rem;
      border-radius: 4px;
      text-decoration: none;
      transition: background 0.2s, color 0.2s;
    }
    .download-btn:hover { background: var(--accent); color: var(--bg); }
    table {
      width: 100%;
      border-collapse: collapse;
      font-family: 'Share Tech Mono', monospace;
      font-size: 0.88rem;
    }
    thead tr { border-bottom: 2px solid var(--border); }
    thead th { padding: 0.6rem 1rem; text-align: left; color: var(--accent); text-transform: uppercase; font-size: 0.75rem; letter-spacing: 0.1em; }
    tbody tr { border-bottom: 1px solid #1a2a3a; transition: background 0.15s; }
    tbody tr:hover { background: #0f1e30; }
    tbody td { padding: 0.5rem 1rem; color: var(--text); }
    .depth-bar-cell { width: 180px; }
    .depth-bar {
      height: 8px;
      background: linear-gradient(90deg, var(--accent), var(--accent2));
      border-radius: 4px;
      min-width: 2px;
    }
  </style>
</head>
<body>
  <header>
    <h1>&#9675; FLOAT DIVE LOG</h1>
    <p>ESP32 Buoyancy Controller &mdash; Post-Mission Data</p>
  </header>
)rawhtml";

  // Parse CSV to build stats and table rows
  float maxDepth = 0;
  float minDepth = 9999;
  int rowCount = 0;
  String tableRows = "";
  // Max depth for bar scaling — use MAX_DEPTH_TARGET as ceiling
  float barMax = 5.0;

  if (file) {
    while (file.available()) {
      String line = file.readStringUntil('\n');
      line.trim();
      if (line.length() == 0) continue;
      int comma = line.indexOf(',');
      if (comma < 0) continue;
      String timeStr = line.substring(0, comma);
      String depthStr = line.substring(comma + 1);
      float d = depthStr.toFloat();
      unsigned long t = timeStr.toInt();
      if (d > maxDepth) maxDepth = d;
      if (d < minDepth) minDepth = d;
      rowCount++;

      int barWidth = (int)((d / barMax) * 160.0);
      if (barWidth < 2) barWidth = 2;

      tableRows += "<tr><td>" + timeStr + "s</td><td>" + depthStr + " m</td><td class='depth-bar-cell'><div class='depth-bar' style='width:" + String(barWidth) + "px'></div></td></tr>\n";
    }
    file.close();
  }

  if (minDepth == 9999) minDepth = 0;

  html += "<div class='stats'>";
  html += "<div class='stat-card'><div class='label'>Samples</div><div class='value'>" + String(rowCount) + "</div></div>";
  html += "<div class='stat-card'><div class='label'>Max Depth</div><div class='value'>" + String(maxDepth, 2) + "m</div></div>";
  html += "<div class='stat-card'><div class='label'>Min Depth</div><div class='value'>" + String(minDepth, 2) + "m</div></div>";
  html += "</div>";

  html += "<a class='download-btn' href='/csv'>&#8659; Download CSV</a>";

  html += "<table><thead><tr><th>Time</th><th>Depth</th><th>Profile</th></tr></thead><tbody>";
  html += tableRows;
  html += "</tbody></table></body></html>";

  server.send(200, "text/html", html);
}

// Serves raw CSV for download
void handleCSV() {
  File file = LittleFS.open(LOG_FILE, FILE_READ);
  if (!file) {
    server.send(404, "text/plain", "No log file found.");
    return;
  }
  server.sendHeader("Content-Disposition", "attachment; filename=dive_log.csv");
  server.streamFile(file, "text/csv");
  file.close();
}

void startWiFi() {
  WiFi.softAP(AP_SSID, (strlen(AP_PASSWORD) > 0) ? AP_PASSWORD : NULL);
  Serial.print("WiFi AP started. Connect to: ");
  Serial.println(AP_SSID);
  Serial.print("Open browser to: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/csv", handleCSV);
  server.begin();
  Serial.println("Webserver running.");
}

void stopWiFi() {
  server.stop();
  WiFi.softAPdisconnect(true);
  Serial.println("WiFi AP stopped. Mission beginning.");
}

// Blocks until no clients are connected, then returns
// Times out after `timeoutSeconds` so the mission isn't held up forever
void serveUntilDisconnect(int timeoutSeconds) {
  Serial.printf("Serving data for up to %ds. Disconnect your device to continue.\n", timeoutSeconds);
  unsigned long start = millis();
  unsigned long lastClient = millis();
  while (millis() - start < (unsigned long)timeoutSeconds * 1000UL) {
    server.handleClient();
    if (WiFi.softAPgetStationNum() > 0) {
      lastClient = millis(); // reset idle timer when someone is connected
    }
    // If a client was connected and has now left, give them 5s to finish loading then move on
    if (millis() - lastClient > 5000 && lastClient != start) break;
    delay(10);
  }
}

// -------------------------------------------------------
// SETUP
// -------------------------------------------------------

void setup() {
  Serial.begin(115200);
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(FWD_PIN, OUTPUT);
  pinMode(REV_PIN, OUTPUT);

  stopActuator();

  if (!LittleFS.begin(true)) Serial.println("LittleFS Mount Fail");

  // --- 1. IMMEDIATE SYSTEM TEST ---
  Serial.println("--- SYSTEM TEST: CLEARING LIMIT SWITCHES ---");
  analogReadResolution(12);

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

  // --- 2. PRE-DIVE WIFI WINDOW ---
  // Turn on WiFi so user can confirm system and clear old data before diving
  startWiFi();
  Serial.println("PRE-DIVE: Connect to WiFi hotspot to review/clear old data.");
  Serial.printf("Profiles planned: %d\n", PROFILE_COUNT);

  // --- 3. 30 SECOND DELAY & LED FLASH ---
  // WiFi stays active during the countdown so user can load the page
  Serial.println("Starting 30s Safety Countdown...");
  unsigned long countdownStart = millis();
  int i = 0;
  while (millis() - countdownStart < 30000UL) {
    server.handleClient(); // keep webserver responsive during countdown
    int flashSpeed = (i < 20) ? 500 : 100;
    digitalWrite(BUILTIN_LED, HIGH);
    delay(flashSpeed);
    server.handleClient();
    digitalWrite(BUILTIN_LED, LOW);
    delay(flashSpeed);
    if (i % 5 == 0) Serial.printf("T-minus: %ds\n", 30 - i);
    i++;
  }

  // Shut WiFi down before entering water
  stopWiFi();
  Serial.println("MISSION INITIATED");
}

// -------------------------------------------------------
// LOOP
// -------------------------------------------------------

void loop() {
  // Mission complete — start post-mission WiFi window
  if (missionState == STATE_DONE) {
    Serial.println("All profiles complete. Starting post-mission WiFi...");
    startWiFi();
    serveUntilDisconnect(300); // serve for up to 5 minutes
    stopWiFi();
    Serial.println("Data served. Powering down webserver. Reset to run again.");
    // Sit idle — reset the board to run another mission
    while (true) {
      digitalWrite(BUILTIN_LED, HIGH); delay(100);
      digitalWrite(BUILTIN_LED, LOW);  delay(100);
    }
  }

  float currentDepth = getDepth();
  if (debug) Serial.printf("Current Depth: %.2f meters\n", currentDepth);

  // Log every second and check USB commands
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
      } else {
        digitalWrite(REV_PIN, HIGH); // Suck water to sink
        digitalWrite(FWD_PIN, LOW);
      }
      break;

    case STATE_RISING:
      if (currentDepth <= TARGET_DEPTH) {
        stopActuator();
        profilesCompleted++;
        Serial.printf("Profile %d/%d complete.\n", profilesCompleted, PROFILE_COUNT);

        if (profilesCompleted >= PROFILE_COUNT) {
          // All done — transition to done state, WiFi starts at top of loop
          missionState = STATE_DONE;
        } else {
          // More profiles to run — go back down
          missionState = STATE_HOVERING;
        }
      } else {
        digitalWrite(FWD_PIN, HIGH); // Expel water to rise
        digitalWrite(REV_PIN, LOW);
      }
      break;

    case STATE_HOVERING:
      if (currentDepth > (TARGET_DEPTH + DEADZONE)) {
        digitalWrite(FWD_PIN, HIGH); // Too deep, push water out
        digitalWrite(REV_PIN, LOW);
      } else if (currentDepth < (TARGET_DEPTH - DEADZONE)) {
        digitalWrite(REV_PIN, LOW);
        digitalWrite(REV_PIN, HIGH); // Too shallow, suck water in
      } else {
        stopActuator();
        // Once stable at target, start next descent
        missionState = STATE_SINKING;
      }
      break;

    case STATE_DONE:
      break; // handled at top of loop()
  }
  delay(100);
}
