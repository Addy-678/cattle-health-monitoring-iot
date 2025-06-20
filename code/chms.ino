#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <math.h>
#include "MAX30100_PulseOximetermega.h"
#include <Adafruit_MLX90614.h>
#include <DHT.h>

// ============= Configuration =============
// ----- Wi-Fi -----
const char* ssid     = "SSID";
const char* password = "PASSWORD";

// ----- NTP for IST (UTC+5:30) -----
const long gmtOffset_sec     = 19800;  // 5.5 hours in seconds
const int  daylightOffset_sec = 0;

// ----- CHMS Endpoints & Credentials -----
const char* healthDataURL = "https://../sendIotData";
const char* motionDataURL = "https://../sendMotionData";
const char* cattle_id     = "134";
const char* device_id     = "1";

// ----- Timing Constants -----
#define STABILIZATION_MS   2000   // 2s sensor stabilization
#define HEALTH_WINDOW_MS   10000  // 10s health data window
#define ADXL_SAMPLE_INT    20     // 50Hz (20ms)

// ----- Sensor Pins -----
#define DHTPIN   4
#define DHTTYPE  DHT21

// ----- Signal Quality Thresholds -----
#define MIN_SPO2      70.0    // Minimum valid SpO₂ (%)
#define MIN_HR        40      // Minimum valid HR (BPM)

// ============= Global State =============
// ----- Sensor Objects -----
Adafruit_MLX90614 mlx = Adafruit_MLX90614();
PulseOximeter     pox;
DHT dht(DHTPIN, DHTTYPE);

// ----- Health Monitoring -----
unsigned long healthWindowTs = 0;
bool stabilizationDone = false;
float sumHRsq         = 0.0;
int   validSamples    = 0;
float maxSpO2         = 0.0;

// ----- Motion Detection (ADXL345) -----
#define ADXL345_ADDRESS 0x53
const int WINDOW_SIZE = 128;  // ~2.56s window
float magBuffer[WINDOW_SIZE];
int   adxlBufferIndex = 0;
enum Behavior { RESTING, STANDING, GRAZING, MOTION };
Behavior currentBehavior = RESTING;
Behavior prevBehavior    = RESTING;
float ax, ay, az;
unsigned long lastAdxlSample = 0;

// ----- Network State -----
volatile bool healthDataReady = false;
String       healthPostData   = "";
volatile bool motionDataReady = false;
String       motionPostData   = "";

// ============= Function Definitions =============
void computeFFT(float* data, int n, float* spectrum) {
  float angleIncrement = 2 * PI / n;
  
  for (int k = 0; k < n / 2; k++) {
    float real = 0, imag = 0;
    float angle = 0;
    
    for (int t = 0; t < n; t++) {
      real += data[t] * cos(angle);
      imag -= data[t] * sin(angle);
      angle += k * angleIncrement;
    }
    spectrum[k] = sqrt(real * real + imag * imag);
  }
}

String getFormattedTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "1970-01-01 00:00:00";
  }
  char timeString[20];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(timeString);
}

void networkTask(void* parameter) {
  for (;;) {
    if (healthDataReady && WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(healthDataURL);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      http.POST(healthPostData);
      http.end();
      healthDataReady = false;
    }

    if (motionDataReady && WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      http.begin(motionDataURL);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      http.POST(motionPostData);
      http.end();
      motionDataReady = false;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void initializeADXL() {
  Wire.beginTransmission(ADXL345_ADDRESS);
  Wire.write(0x2C);  // BW_RATE register
  Wire.write(0x0B);  // 50Hz output rate
  Wire.write(0x2D);  // Power Control
  Wire.write(0x08);  // Measurement mode
  Wire.endTransmission();
}

// ============= Main Functions =============
void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000);  // Fast I²C (400kHz)

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) delay(500);
  Serial.println("\nConnected to Wi-Fi");

  // Initialize NTP
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");

  // Initialize sensors
  pox.begin();
  pox.setIRLedCurrent(MAX30100_LED_CURR_27_1MA);
  mlx.begin();
  dht.begin();
  initializeADXL();

  Serial.println("Waiting 2s for sensor stabilization...");

  // Start network task on core 1
  xTaskCreatePinnedToCore(
    networkTask, "NetworkTask", 4096, NULL, 1, NULL, 1
  );
}

void processHealthData() {
  unsigned long now = millis();
  
  // 1. Check stabilization period
  if (!stabilizationDone) {
    if (now >= STABILIZATION_MS) {
      stabilizationDone = true;
      Serial.println("Sensors stabilized. Starting 10s windows...");
    }
    return;
  }

  // 2. Start first window immediately after stabilization
  if (healthWindowTs == 0) {
    healthWindowTs = now;
    Serial.println("Starting first 10s window");
  }

  // 3. Check for valid readings
  float instSpO2 = pox.getSpO2();
  int instHR = round(pox.getHeartRate());
  
  if (instSpO2 >= MIN_SPO2 && instHR >= MIN_HR) {
    validSamples++;
    sumHRsq += pow(instHR, 2);
    if (instSpO2 > maxSpO2) maxSpO2 = instSpO2;
  }

  // 4. Process window after 10 seconds
  if (now - healthWindowTs >= HEALTH_WINDOW_MS) {
    int finalHR = 0;
    float finalSpO2 = 0.0;

    // Only compute values if we have valid samples
    if (validSamples > 0) {
      finalHR = round(sqrt(sumHRsq / validSamples));
      finalSpO2 = maxSpO2;
    }

    // 5. Read temperature sensors
    Wire.setClock(100000);
    float rawBodyTemp = mlx.readObjectTempC();
    Wire.setClock(400000);
    float envTempC = dht.readTemperature();
    float envHumidity = dht.readHumidity();
    float rectalTemp = 0.82f * rawBodyTemp + 0.13f * envTempC + 0.03f * envHumidity + 2.1f;

    // 6. Print diagnostics
    Serial.println("\n===== Health Data =====");
    Serial.printf("SpO₂: %.1f%% | HR: %d BPM\n", finalSpO2, finalHR);
    Serial.printf("Body: %.1f°C | Env: %.1f°C | Hum: %.1f%%\n", rawBodyTemp, envTempC, envHumidity);
    Serial.printf("Rectal: %.1f°C | Valid Samples: %d\n", rectalTemp, validSamples);

    // 7. Prepare for transmission
    String measuredAt = getFormattedTime();
    healthPostData = "cattle_id=" + String(cattle_id) +
                     "&device_id=" + String(device_id) +
                     "&measured_at=" + measuredAt +
                     "&data=" + String(rawBodyTemp, 2) + "," +
                     String(finalSpO2, 2) + "," +
                     String(finalHR) + "," +
                     String(envTempC, 2) + "," +
                     String(envHumidity, 2) + "," +
                     String(rectalTemp, 2);
    healthDataReady = true;

    // 8. Reset for next window
    healthWindowTs = now;
    sumHRsq = 0.0;
    maxSpO2 = 0.0;
    validSamples = 0;
  }
}

void processMotionData() {
  if (millis() - lastAdxlSample < ADXL_SAMPLE_INT) return;
  lastAdxlSample = millis();

  // Read accelerometer data
  Wire.beginTransmission(ADXL345_ADDRESS);
  Wire.write(0x32);
  Wire.endTransmission(false);
  if (Wire.requestFrom(ADXL345_ADDRESS, 6) == 6) {
    int16_t x = Wire.read() | (Wire.read() << 8);
    int16_t y = Wire.read() | (Wire.read() << 8);
    int16_t z = Wire.read() | (Wire.read() << 8);
    
    ax = x / 256.0f;
    ay = y / 256.0f;
    az = z / 256.0f;
    
    // Store magnitude in circular buffer
    magBuffer[adxlBufferIndex] = sqrt(ax*ax + ay*ay + az*az);
    adxlBufferIndex = (adxlBufferIndex + 1) % WINDOW_SIZE;

    // Process when buffer is full
    if (adxlBufferIndex == 0) {
      float sum = 0, sumSq = 0, maxMag = 0;
      for (int i = 0; i < WINDOW_SIZE; i++) {
        sum += magBuffer[i];
        sumSq += magBuffer[i] * magBuffer[i];
        if (magBuffer[i] > maxMag) maxMag = magBuffer[i];
      }
      
      float mean = sum / WINDOW_SIZE;
      float stdDev = sqrt((sumSq / WINDOW_SIZE) - (mean * mean));
      
      // Frequency analysis
      float spectrum[WINDOW_SIZE / 2];
      computeFFT(magBuffer, WINDOW_SIZE, spectrum);
      int maxIndex = 1;
      for (int i = 2; i < WINDOW_SIZE/2; i++) {
        if (spectrum[i] > spectrum[maxIndex]) maxIndex = i;
      }
      float dominantFreq = maxIndex * (50.0f / WINDOW_SIZE);

      // Classify behavior
      if (dominantFreq < 0.5f && maxMag < 1.1f && stdDev < 0.04f) {
        currentBehavior = RESTING;
      } else if (dominantFreq < 1.2f && maxMag < 1.3f && stdDev < 0.08f) {
        currentBehavior = STANDING;
      } else if (dominantFreq <= 2.8f && stdDev < 0.2f) {
        currentBehavior = GRAZING;
      } else {
        currentBehavior = MOTION;
      }

      // Notify on behavior change
      if (currentBehavior != prevBehavior) {
        Serial.printf("\nMotion: %.2fHz | Mag: %.2fg | Std: %.3f\n", 
                      dominantFreq, maxMag, stdDev);
        Serial.print("Behavior: ");
        switch(currentBehavior) {
          case RESTING:  Serial.println("RESTING"); break;
          case STANDING: Serial.println("STANDING"); break;
          case GRAZING:  Serial.println("GRAZING"); break;
          default:       Serial.println("MOTION"); break;
        }
        
        motionPostData = "cattle_id=" + String(cattle_id) +
                         "&device_id=" + String(device_id) +
                         "&stdDev=" + String(stdDev, 2) +
                         "&maxMagnitude=" + String(maxMag, 2) +
                         "&domFrequency=" + String(dominantFreq, 2);
        motionDataReady = true;
        prevBehavior = currentBehavior;
      }
    }
  }
}

void loop() {
  pox.update();  // Critical for MAX30100 operation
  processHealthData();
  processMotionData();
}
