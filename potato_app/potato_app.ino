#include <DHT.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "LAB RPL";
const char* password = "@polije.ac.id";

// Flask server IP and endpoint
const char* serverIP = "172.16.115.93";  // IP dari Flask server
const int serverPort = 5000;
const char* endpointSet = "/set";  // Endpoint untuk mengirim data
const char* endpointGet = "/get";  // Endpoint untuk mendapatkan data
const char* endpointDevice = "/device/1730184375";  // Endpoint untuk mendapatkan data

#define LDR_PIN A0         // Pin analog untuk LDR
#define DHT_PIN D4         // Pin untuk DHT22
#define DHT_TYPE DHT22
#define RELAY_PIN D2       // Pin untuk relay

DHT dht(DHT_PIN, DHT_TYPE);
String mode;       // Default ke auto
String blower_status;
float temperature_threshold;

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  
  dht.begin();

  mode = "manual";
  blower_status = "OFF";
  temperature_threshold = 30.0;

  connectToWiFi();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    readAndDisplaySensorData();
    getModeAndBlowerStatus();  // Ambil mode dan blower_status dari server
    controlRelay(mode, temperature_threshold);            // Kontrol relay sesuai mode
    sendToServer();            // Kirim data sensor ke server
  } else {
    Serial.println("WiFi not connected!");
  }
  
  delay(2000);  // Delay 10 detik sebelum pembacaan berikutnya
}

// Fungsi untuk menghubungkan ke WiFi
void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi.");
}

// Fungsi untuk membaca data sensor dan menampilkannya
void readAndDisplaySensorData() {
  // Membaca data dari LDR
  int ldrValue = analogRead(LDR_PIN);

  // Invers untuk membalik logika
  int inversLdrValue = 1024 - ldrValue; // 1023 adalah maksimum nilai ADC
  Serial.print("Nilai LDR (Invers): ");
  Serial.println(inversLdrValue);


  // Membaca data dari sensor DHT22
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Menampilkan data suhu dan kelembaban
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.println("°C");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
  Serial.println("");
}

// Fungsi untuk mengatur relay berdasarkan mode
void controlRelay(String mode, float temperature_threshold) {
  float temperature = dht.readTemperature();

  if (mode == "auto") {
    // Mode auto: kontrol relay berdasarkan suhu
    if (temperature > temperature_threshold) {
      digitalWrite(RELAY_PIN, LOW);  // Relay aktif (blower aktif)
      Serial.println("Mode Auto: Relay ON (Blower aktif)");
    } else {
      digitalWrite(RELAY_PIN, HIGH);   // Relay mati (blower mati)
      Serial.println("Mode Auto: Relay OFF (Blower mati)");
    }
    Serial.println("auto");
  } else if (mode == "manual") {
    // Mode manual: kontrol relay berdasarkan blower_status
    if (blower_status == "ON") {
      digitalWrite(RELAY_PIN, LOW);  // Relay aktif (blower aktif)
      Serial.println("Mode Manual: Relay ON (Blower aktif)");
    } else if (blower_status == "OFF"){
      digitalWrite(RELAY_PIN, HIGH);   // Relay mati (blower mati)
      Serial.println("Mode Manual: Relay OFF (Blower mati)");
    }
    Serial.println("manual");
  } else {
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("elseelse");
    Serial.print("mode : ");
    Serial.println(mode);
    Serial.print("blower status : ");
    Serial.println(blower_status);
  }
    Serial.print("mode : ");
    Serial.println(mode);
    Serial.print("blower status : ");
    Serial.println(blower_status);
}

// Fungsi untuk mengambil mode dan blower_status dari server
void getModeAndBlowerStatus() {
  HTTPClient http;
  WiFiClient client;

  // HTTP request ke Flask API dengan parameter device
  String url = String("http://") + serverIP + ":" + String(serverPort) + "/get?device=1730184375";
  http.begin(client, url); // Inisialisasi request
  int httpResponseCode = http.GET(); // Mengirim request GET

  if (httpResponseCode > 0) {
    String response = http.getString();
    Serial.println("Response: " + response);

    // Gunakan ArduinoJson untuk parsing JSON
    const size_t capacity = JSON_OBJECT_SIZE(3) + 60; // Sesuaikan kapasitas buffer
    DynamicJsonDocument doc(capacity);

    // Parsing JSON response
    DeserializationError error = deserializeJson(doc, response);
    if (error) {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());
      return;
    }

    // Ambil nilai mode dan blower_status dari JSON
    mode = doc["mode"].as<String>();
    blower_status = doc["blower_status"].as<String>();
    
    // Ambil temperature_threshold dari auto_conditions
    JsonObject auto_conditions = doc["auto_conditions"];
    if (!auto_conditions.isNull()) {
      temperature_threshold = auto_conditions["temperature_threshold"].as<float>();
    }

    Serial.println("Parsed Mode: " + mode);
    Serial.println("Parsed Blower Status: " + blower_status);
    Serial.print("Parsed Temperature Threshold: ");
    Serial.println(temperature_threshold);
  } else {
    Serial.println("Failed to get data. Error code: " + String(httpResponseCode));
  }

  http.end();
}

void sendToServer() {
  HTTPClient http;
  WiFiClient client;

  // Membaca data dari sensor
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int ldrValue = analogRead(LDR_PIN);
  int inversLdrValue = 1024 - ldrValue;

  // Periksa apakah data sensor valid
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // JSON payload
  String payload = "{\"device\": \"1730184375\", \"humidity\": " + String(humidity) +
                   ", \"light_intensity\": " + String(inversLdrValue) +
                   ", \"temperature\": " + String(int(temperature)) + "}";

  // HTTP request ke Flask API
  String url = String("http://") + serverIP + ":" + String(serverPort) + endpointSet; // Gunakan endpointSet

  http.begin(client, url);                      // Mulai koneksi HTTP
  http.addHeader("Content-Type", "application/json");  // Header untuk JSON
  int httpResponseCode = http.POST(payload);   // Kirim data POST

  // Periksa respons dari server
  if (httpResponseCode > 0) {
    // Serial.println("Data sent successfully. Response code: " + String(httpResponseCode));
    String response = http.getString();
    Serial.println("Response: " + response);
  } else {
    Serial.println("Failed to send data. Error code: " + String(httpResponseCode));
  }

  http.end(); // Akhiri koneksi
}

