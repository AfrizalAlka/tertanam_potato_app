#include <DHT.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
// #include <FirebaseESP8266.h>

// WiFi credentials
// const char* ssid = "mahen kecil";
// const char* password = "1231231234";
// const char* ssid = "LAB MOBILE";
// const char* password = "Pr0gram1ng";
const char* ssid = "Realme 11 Pro Plus";
const char* password = "5ghzgamuncul";

// Flask server IP and endpoint
const char* serverIP = "192.168.0.102";  // IP dari Flask server
const int serverPort = 5000;
const char* endpointSet = "/set";  // Endpoint untuk mengirim data
const char* endpointGet = "/get";  // Endpoint untuk mendapatkan data
const char* endpointDevice = "/device/1730184375";  // Endpoint untuk mendapatkan data

#define LDR_PIN A0         // Pin analog untuk LDR
#define DHT_PIN D4         // Pin untuk DHT22
#define DHT_TYPE DHT22
#define RELAY_PIN D2       // Pin untuk relay
// #define FIREBASE_HOST "https://potato-base-34d80-default-rtdb.asia-southeast1.firebasedatabase.app/"
// #define FIREBASE_AUTH "RL1Nhzm9Isoia0I9hQPehQ3Ni2ecrUudyu2iR3u4"

// FirebaseData firebaseData;
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

  // Inisialisasi Firebase
  // Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);

  // // Set listener untuk mode dan blower_status
  // Firebase.setStreamCallback(firebaseData, streamCallback, streamTimeoutCallback);
  // Firebase.beginStream(firebaseData, "/device/1730184375");
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
  Serial.print("Nilai LDR: ");
  Serial.println(ldrValue);

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

  // Periksa apakah data sensor valid
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // JSON payload
  String payload = "{\"device\": \"1730184375\", \"humidity\": " + String(humidity) +
                   ", \"light_intensity\": " + String(ldrValue) +
                   ", \"temperature\": " + String(int(temperature)) + "}";

  // HTTP request ke Flask API
  String url = String("http://") + serverIP + ":" + String(serverPort) + endpointSet; // Gunakan endpointSet saja
  // Serial.println("Sending data to: " + url);
  // Serial.println("Payload: " + payload);

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

// // Callback jika ada perubahan data di Firebase
// void streamCallback(FirebaseStream data) {
//   if (data.getDataType() == "json") {
//     mode = data.jsonData()["mode"].as<String>();
//     blower_status = data.jsonData()["blower_status"].as<String>();
//     Serial.println("Mode: " + mode + ", Blower Status: " + blower_status);
//     controlRelay();  // Eksekusi kontrol relay langsung
//   }
// }

// void streamTimeoutCallback(bool timeout) {
//   if (timeout) {
//     Serial.println("Stream timeout, reconnecting...");
//     Firebase.beginStream(firebaseData, "/device/1730184375");
//   }
// }
