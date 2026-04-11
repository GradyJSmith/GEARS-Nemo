/* 
 * ESP32 Submersible Pressure Sensor (0-5m) 
 * Calibrated for feet with high-resolution averaging
 */

const int sensorPin = 34; // Pin D34 (or GPIO 34)
const float VREF = 3.3;   // ESP32 Voltage
const int ADC_RES = 4095; // 12-bit ADC

// --- CALIBRATION SETTINGS ---
const float VOLT_AT_0M = 0.340;   // YOUR measured dry voltage
const float VOLT_AT_5M = 2.400;   // Expected voltage at 5m depth (20mA)
const float MAX_DEPTH_FT = 16.40; // 5 meters converted to feet

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  Serial.println("System Initialized. Place sensor in water to begin.");
}

void loop() {
  // High-sample averaging to smooth out ESP32 ADC jitter
  float sum = 0;
  int samples = 500; 
  for(int i = 0; i < samples; i++) {
    sum += analogRead(sensorPin);
    delayMicroseconds(100); 
  }
  float avgADC = sum / (float)samples;

  // Convert ADC to Voltage
  float voltage = (avgADC * VREF) / (float)ADC_RES;

  // Calculate Depth in Feet
  // Formula: (CurrentV - ZeroV) * (TotalRange / VoltageSpan)
  float depthFt = (voltage - VOLT_AT_0M) * (MAX_DEPTH_FT / (VOLT_AT_5M - VOLT_AT_0M));

  // Safety: Don't show negative depth
  if (depthFt < 0.05) depthFt = 0.00; 

  // Output to Serial
  Serial.print("ADC: "); Serial.print(avgADC, 1);
  Serial.print(" | V: "); Serial.print(voltage, 4);
  Serial.print(" | Depth: "); Serial.print(depthFt, 2);
  Serial.println(" ft");

  delay(250); 
}
