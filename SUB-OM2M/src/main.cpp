#include "WiFi.h"
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>

#define PORT 8100
#define BAUD 115200

// WiFi credentials
const char *ssid = "SCRC_LAB_IOT";
const char *password = "Scrciiith@123";

// Web server object
AsyncWebServer server(PORT);

// LED Pin Definitions
const int LED_PINS[] = {13, 14, 27, 26, 25, 33, 32};
const int NUM_LEDS = sizeof(LED_PINS) / sizeof(LED_PINS[0]);
const int wifiLed = 2;

// LED Mapping to Identifiers
const char *LED_IDS[] = {"L026", "L001", "L002", "L003", "L004", "L005", "L014"};

// List of URLs to fetch initial LED states
const char *urls[] = {
    "http://dev-onem2m.iiit.ac.in:443/~/in-cse/in-name/AE-WN/WN-L026-01/Status/la",
    "http://dev-onem2m.iiit.ac.in:443/~/in-cse/in-name/AE-WN/WN-L001-03/Status/la",
    "http://dev-onem2m.iiit.ac.in:443/~/in-cse/in-name/AE-WN/WN-L002-02/Status/la",
    "http://dev-onem2m.iiit.ac.in:443/~/in-cse/in-name/AE-WN/WN-L003-02/Status/la",
    "http://dev-onem2m.iiit.ac.in:443/~/in-cse/in-name/AE-WN/WN-L004-02/Status/la",
    "http://dev-onem2m.iiit.ac.in:443/~/in-cse/in-name/AE-WN/WN-L005-02/Status/la",
    "http://dev-onem2m.iiit.ac.in:443/~/in-cse/in-name/AE-WN/WN-L014-01/Status/la"};
const int num_urls = sizeof(urls) / sizeof(urls[0]);

void wifi_init()
{
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(wifiLed, HIGH);
    delay(100);
    digitalWrite(wifiLed, LOW);
    delay(100);
  }
  Serial.println("\nConnected to WiFi");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

// Function to fetch initial LED states from the server
void fetchDataFromURL(const char *url, int ledIndex)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    HTTPClient http;
    http.begin(url);
    http.addHeader("X-M2M-Origin", "Tue_20_12_22:Tue_20_12_22");
    http.addHeader("Content-Type", "application/json");

    int httpCode = http.GET();
    if (httpCode > 0)
    {
      String payload = http.getString();
      // Serial.println("Raw JSON:");
      // Serial.println(payload);

      // JSON Parsing
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);

      if (!error)
      {
        String conValue = doc["m2m:cin"]["con"].as<String>();
        Serial.print("Extracted con value for LED ");
        Serial.println(LED_IDS[ledIndex]);

        String ledID = conValue.substring(0, 4);
        String ledState = conValue.substring(4);
        ledState.trim();

        Serial.print("Extracted LED ID: ");
        Serial.println(ledID);
        Serial.print("Extracted LED State: ");
        Serial.println("'" + ledState + "'");

                if (ledState.equals("ON"))
        {
          digitalWrite(LED_PINS[ledIndex], HIGH);
          Serial.println(LED_PINS[ledIndex]);
        }
        else if (ledState.equals("OFF"))
        {
          Serial.println(LED_PINS[ledIndex]);
          digitalWrite(LED_PINS[ledIndex], LOW);
        }
      }
      else
      {
        Serial.print("JSON Parsing Error: ");
        Serial.println(error.c_str());
      }
    }
    else
    {
      Serial.print("HTTP Error: ");
      Serial.println(httpCode);
    }
    http.end();
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
}

// Function to handle POST request data
void data_receive(AsyncWebServerRequest *request, unsigned char *data, size_t len, size_t index, size_t total)
{
  String receivedData;
  for (size_t i = 0; i < len; i++)
  {
    receivedData += (char)data[i];
  }

  // Parse JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, receivedData);
  if (error)
  {
    Serial.print("JSON Parse Failed: ");
    Serial.println(error.c_str());
    request->send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
    return;
  }

  JsonVariant conValue = doc["m2m:sgn"]["m2m:nev"]["m2m:rep"]["m2m:cin"]["con"];
  if (!conValue)
  {
    Serial.println("Error: 'con' field not found.");
    request->send(400, "application/json", "{\"error\": \"Missing 'con' field\"}");
    return;
  }

  String conString = conValue.as<String>();
  Serial.println("\nReceived 'con' Value: " + conString);

  // Extract LED ID and State
  String ledID = conString.substring(0, 4);
  String ledState = conString.substring(4);
  ledState.trim();

  Serial.print("Extracted LED ID: ");
  Serial.println(ledID);
  Serial.print("Extracted LED State: ");
  Serial.println("'" + ledState + "'");

  // Match LED ID and Control Corresponding LED
  for (int i = 0; i < NUM_LEDS; i++)
  {
    if (ledID.equals(LED_IDS[i]))
    {
      Serial.print("Matched ID: ");
      Serial.println(ledID);
      Serial.print("Changing LED on GPIO ");
      Serial.println(LED_PINS[i]);

      if (ledState.equals("ON"))
      {
        digitalWrite(LED_PINS[i], HIGH);
        Serial.println("LED TURNED ON");
      }
      else if (ledState.equals("OFF"))
      {
        digitalWrite(LED_PINS[i], LOW);
        Serial.println("LED TURNED OFF");
      }
      break;
    }
  }

  request->send(200, "application/json", "{\"status\": \"Data received\"}");
}

void setup()
{
  Serial.begin(BAUD);

  // Initialize LED pins as output
  for (int i = 0; i < NUM_LEDS; i++)
  {
    pinMode(LED_PINS[i], OUTPUT);
    digitalWrite(LED_PINS[i], LOW); // Ensure all LEDs are OFF initially
  }
  pinMode(wifiLed, OUTPUT);

  wifi_init();

  // Fetch initial LED states
  for (int i = 0; i < num_urls; i++)
  {
    fetchDataFromURL(urls[i], i);
    delay(2000); // Small delay between requests
  }

  // Start server and listen for POST requests
  server.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, data_receive);
  server.begin();
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("WiFi Disconnected! Reconnecting...");
    WiFi.disconnect();
    WiFi.reconnect();
    delay(5000);
  }
  delay(1000);
}
