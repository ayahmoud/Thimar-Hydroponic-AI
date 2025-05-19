#include <WiFi.h>
#include <WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// 🔹 Wi-Fi Credentials
const char* ssid = "MyHouse";  // Replace with your Wi-Fi SSID
const char* password = "wx7a52a5";  // Replace with your Wi-Fi password

// 🔹 Static IP Configuration
IPAddress local_IP(192, 168, 1, 200);  
IPAddress gateway(192, 168, 0, 1);     
IPAddress subnet(255, 255, 255, 0);    
IPAddress primaryDNS(8, 8, 8, 8);       
IPAddress secondaryDNS(8, 8, 4, 4);    

// 🔹 Set up Web Server
WebServer server(80); // HTTP server runs on port 80

// 🔹 Sensor Pins
const int oneWireBus = 14;  // DS18B20 GPIO
#define TdsSensorPin 35
#define VREF 3.3  // ADC Reference Voltage
#define NUM_READINGS 10  // Number of readings for averaging

OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);

void setup() {
    Serial.begin(115200);
    delay(1000);  // Give time for Serial Monitor to start
    Serial.println("\n🚀 ESP32 Static IP + Web Server Starting...");

    // 🔹 Try to assign Static IP
    Serial.println("⚡ Configuring static IP...");
    if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
        Serial.println("❌ Failed to configure Static IP! Using dynamic IP.");
    } else {
        Serial.println("✅ Static IP Configured Successfully!");
    }

    WiFi.begin(ssid, password);
    Serial.print("⏳ Connecting to Wi-Fi");

    int timeout = 30;  // Wait for max 30 seconds
    while (WiFi.status() != WL_CONNECTED && timeout > 0) {
        delay(1000);
        Serial.print(".");
        timeout--;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ Connected to Wi-Fi!");
        Serial.print("📡 ESP32 Static IP Address: ");
        Serial.println(WiFi.localIP());  // Print the Static IP Address
    } else {
        Serial.println("\n❌ Failed to connect to Wi-Fi! Check SSID/Password.");
    }

    sensors.begin();

    // 🔹 Define API endpoint for sensor readings
    server.on("/getSensorData", HTTP_GET, []() {
        float avgTemperature, avgTDS;
        readSensorData(&avgTemperature, &avgTDS);

        // Create JSON response
        String json = "{ \"temperature\": " + String(avgTemperature, 2) +
                      ", \"tds\": " + String(avgTDS, 1) + " }";

        server.send(200, "application/json", json);
        Serial.println("📡 Sent Data to App: " + json);
    });

    server.begin();
    Serial.println("✅ HTTP Server Started!");
}

void loop() {
    server.handleClient(); // Handle API requests
}

// 🔹 Function to Read & Average Sensor Data
void readSensorData(float* avgTemperature, float* avgTDS) {
    float totalTemperature = 0;
    float totalTDS = 0;

    for (int i = 0; i < NUM_READINGS; i++) {
        sensors.requestTemperatures();
        float temperature = sensors.getTempCByIndex(0);
        totalTemperature += temperature;

        int analogValue = analogRead(TdsSensorPin);
        float voltage = analogValue * (VREF / 4095.0);
        float compensationCoefficient = 1.0 + 0.02 * (temperature - 25.0);
        float compensationVoltage = voltage / compensationCoefficient;

        float tdsValue = (133.42 * compensationVoltage * compensationVoltage * compensationVoltage
                         - 255.86 * compensationVoltage * compensationVoltage
                         + 857.39 * compensationVoltage) * 0.5;

        totalTDS += tdsValue;

        delay(100);  // Small delay between readings
    }

    // 🔹 Store averaged values in pointers
    *avgTemperature = totalTemperature / NUM_READINGS;
    *avgTDS = totalTDS / NUM_READINGS;
}
