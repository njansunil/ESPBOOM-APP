
/*
  =====================================================
                  ESPBOOM FIRMWARE
  =====================================================

  Created & Developed By : SUNILKUMAR
  Project / Brand        : LUMINOSCOPE
  Android Application    : ESPBOOM

  Hardware:
  - ESP32 DevKit
  - SSD1306 OLED Display (128 x 64)
  - I2C Address: 0x3C

  OLED Pins:
  - SDA: GPIO 21
  - SCL: GPIO 22

  Communication:
  - Wi-Fi Phone Hotspot
  - TCP Port: 8080
  - Frame Size: 1024 bytes
  - Frame Acknowledgement: 'K'

  Instagram:
  https://www.instagram.com/luminoscope.yt/

  YouTube:
  LUMINOSCOPE YT
  Channel URL: ADD_YOUR_YOUTUBE_CHANNEL_URL_HERE

  Description:
  Receives monochrome video frames from the ESPBOOM
  Android app over Wi-Fi and displays them on the
  SSD1306 OLED display.

  Copyright (c) 2026 SUNILKUMAR
  =====================================================
*/

#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================================================
// Wi-Fi Hotspot Settings
// =====================================================

const char* WIFI_SSID = "Enter Your SSID";
const char* WIFI_PASSWORD = "Enter Your Password ";

// =====================================================
// OLED Configuration
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define OLED_SDA 21
#define OLED_SCL 22
#define OLED_ADDR 0x3C
#define OLED_RESET -1

// =====================================================
// TCP Server Configuration
// =====================================================

#define TCP_PORT 8080
#define FRAME_SIZE 1024

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

WiFiServer server(TCP_PORT);

uint8_t frameBuffer[FRAME_SIZE];

// =====================================================
// Receive One Complete OLED Frame
// =====================================================

bool readExactFrame(WiFiClient& client) {
  size_t received = 0;
  unsigned long lastDataTime = millis();

  while (received < FRAME_SIZE && client.connected()) {
    int availableBytes = client.available();

    if (availableBytes > 0) {
      size_t remaining = FRAME_SIZE - received;

      size_t amount = min(
        remaining,
        (size_t)availableBytes
      );

      int count = client.read(
        frameBuffer + received,
        amount
      );

      if (count > 0) {
        received += count;
        lastDataTime = millis();
      }
    } else {
      if (millis() - lastDataTime > 5000) {
        return false;
      }

      delay(1);
    }
  }

  return received == FRAME_SIZE;
}

// =====================================================
// Setup
// =====================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  // Initialize OLED I2C
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDR
      )) {
    Serial.println("OLED initialization failed!");

    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);

  display.println("ESPBOOM");
  display.println("By SUNILKUMAR");
  display.println("Connecting WiFi...");
  display.display();

  // Connect to phone hotspot
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to hotspot");

  unsigned long startTime = millis();

  while (
    WiFi.status() != WL_CONNECTED &&
    millis() - startTime < 30000
  ) {
    Serial.print(".");
    delay(500);
  }

  display.clearDisplay();
  display.setCursor(0, 0);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nWiFi connection failed!");

    display.println("WiFi FAILED!");
    display.println("Check hotspot");
    display.display();

    return;
  }

  // Show connection details
  Serial.println("\nWiFi connected!");
  Serial.print("ESP32 IP Address: ");
  Serial.println(WiFi.localIP());

  display.println("ESPBOOM READY");
  display.println();
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.println();
  display.println("Waiting for app...");
  display.display();

  // Start TCP server
  server.begin();
  server.setNoDelay(true);

  Serial.print("TCP server port: ");
  Serial.println(TCP_PORT);
}

// =====================================================
// Main Loop
// =====================================================

void loop() {
  // Wait for Wi-Fi connection
  if (WiFi.status() != WL_CONNECTED) {
    delay(100);
    return;
  }

  // Wait for Android app
  WiFiClient client = server.available();

  if (!client) {
    delay(1);
    return;
  }

  Serial.println("ESPBOOM connected!");

  client.setNoDelay(true);

  while (client.connected()) {
    if (client.available() == 0) {
      delay(1);
      continue;
    }

    // Receive exactly 1024 bytes
    if (!readExactFrame(client)) {
      Serial.println(
        "Frame receive timeout or disconnect"
      );

      break;
    }

    // Copy the received SSD1306 frame into the
    // display buffer and refresh the OLED.
    memcpy(
      display.getBuffer(),
      frameBuffer,
      FRAME_SIZE
    );

    display.display();

    // Acknowledge the frame after display update
    if (client.connected()) {
      client.write((uint8_t)'K');
    }
  }

  client.stop();

  Serial.println("ESPBOOM disconnected");
}
